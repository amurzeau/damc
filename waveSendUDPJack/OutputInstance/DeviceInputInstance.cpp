#include "DeviceInputInstance.h"
#include <stdio.h>

std::vector<std::string> DeviceInputInstance::getDeviceList() {
	std::vector<std::string> result;
	int numDevices = Pa_GetDeviceCount();

	for(int i = 0; i < numDevices; i++) {
		const PaDeviceInfo* deviceInfo = Pa_GetDeviceInfo(i);
		std::string name;

		if(deviceInfo->maxInputChannels <= 0)
			continue;

		name = std::string(Pa_GetHostApiInfo(deviceInfo->hostApi)->name) + "::" + std::string(deviceInfo->name);
		result.push_back(name);
	}

	return result;
}

int DeviceInputInstance::getDeviceIndex(const std::string& name) {
	int numDevices = Pa_GetDeviceCount();

	if(name == "default_out") {
		return Pa_GetDefaultOutputDevice();
	} else if(name == "default_in") {
		return Pa_GetDefaultInputDevice();
	}

	for(int i = 0; i < numDevices; i++) {
		const PaDeviceInfo* deviceInfo = Pa_GetDeviceInfo(i);
		std::string devName;

		devName = std::string(Pa_GetHostApiInfo(deviceInfo->hostApi)->name) + "::" + std::string(deviceInfo->name);
		if(devName == name)
			return i;
	}

	return -1;
}

void DeviceInputInstance::stop() {
	if(stream) {
		Pa_StopStream(stream);
		Pa_CloseStream(stream);
	}
	stream = nullptr;
}

const char* DeviceInputInstance::getName() {
	return "device";
}

int DeviceInputInstance::start(int index, size_t numChannel, int sampleRate, int jackBufferSize) {
	PaStreamParameters inputParameters;
	int inputDeviceIndex;

	stream = nullptr;

	inputDeviceIndex = getDeviceIndex(inputDevice);

	if(inputDeviceIndex < 0 || inputDeviceIndex >= Pa_GetDeviceCount()) {
		printf("Bad portaudio input device %s\n", inputDevice.c_str());
		return paInvalidDevice;
	}

	resampledBuffer.reserve(deviceSampleRate);
	resamplingFilters.resize(numChannel);
	ringBuffers.clear();
	for(size_t i = 0; i < numChannel; i++) {
		std::unique_ptr<jack_ringbuffer_t, void (*)(jack_ringbuffer_t*)> buffer(nullptr, &jack_ringbuffer_free);
		buffer.reset(jack_ringbuffer_create(jackBufferSize * 5 * sizeof(jack_default_audio_sample_t)));
		ringBuffers.emplace_back(std::move(buffer));

		resamplingFilters[i].reset(deviceSampleRate);
		resamplingFilters[i].setClockDrift(this->clockDrift);
		resamplingFilters[i].setTargetSamplingRate(sampleRate);
	}

	inputParameters.device = inputDeviceIndex;
	inputParameters.channelCount = numChannel;
	inputParameters.sampleFormat = paFloat32 | paNonInterleaved;  // 32 bit floating point output
	inputParameters.suggestedLatency = Pa_GetDeviceInfo(inputParameters.device)->defaultLowInputLatency;
	inputParameters.hostApiSpecificStreamInfo = NULL;

	int ret = Pa_OpenStream(&stream,
	                        &inputParameters,
	                        nullptr,
	                        deviceSampleRate,
	                        paFramesPerBufferUnspecified,
	                        paClipOff | paDitherOff,
	                        &renderCallbackStatic,
	                        this);
	if(ret != paNoError) {
		printf("Portaudio open error: %s(%d), device: %s::%s\n",
		       Pa_GetErrorText(ret),
		       ret,
		       Pa_GetHostApiInfo(Pa_GetDeviceInfo(inputDeviceIndex)->hostApi)->name,
		       Pa_GetDeviceInfo(inputDeviceIndex)->name);
		return ret;
	}

	printf("Using output device %d %s, %s with latency %.3f\n",
	       inputDeviceIndex,
	       Pa_GetHostApiInfo(Pa_GetDeviceInfo(inputDeviceIndex)->hostApi)->name,
	       Pa_GetDeviceInfo(inputDeviceIndex)->name,
	       Pa_GetStreamInfo(stream)->outputLatency);

	bufferLatencyMeasurePeriodSize = 60 * sampleRate / jackBufferSize;
	bufferLatencyNr = 0;
	bufferLatencyHistory.clear();
	bufferLatencyHistory.reserve(bufferLatencyMeasurePeriodSize);
	previousAverageLatency = 0;
	clockDriftPpm = 0;
	isPaRunning = false;
	printf("Using buffer size %d, device sample rate: %d\n", jackBufferSize, (int) deviceSampleRate);

	Pa_StartStream(stream);

	return ret;
}

void DeviceInputInstance::setParameters(const nlohmann::json& json) {
	inputDevice = json.value("device", inputDevice);
	float newSampleRate = json.value("sampleRate", 0);
	if(newSampleRate) {
		deviceSampleRate = newSampleRate;
	}

	auto clockDrift = json.find("clockDrift");
	if(clockDrift != json.end()) {
		this->clockDrift = clockDrift.value().get<float>();
	}

	for(auto& resamplingFilter : resamplingFilters) {
		resamplingFilter.setClockDrift(this->clockDrift);
		resamplingFilter.setTargetSamplingRate(this->deviceSampleRate);
	}
}

nlohmann::json DeviceInputInstance::getParameters() {
	return nlohmann::json::object(
	    {{"device", inputDevice}, {"clockDrift", clockDrift}, {"sampleRate", deviceSampleRate}});
}

int DeviceInputInstance::renderCallbackStatic(const void* input,
                                              void* output,
                                              unsigned long frameCount,
                                              const PaStreamCallbackTimeInfo* timeInfo,
                                              PaStreamCallbackFlags statusFlags,
                                              void* userData) {
	DeviceInputInstance* thisInstance = (DeviceInputInstance*) userData;
	size_t numChannel = thisInstance->ringBuffers.size();
	const jack_default_audio_sample_t* const* inputBuffers = (const jack_default_audio_sample_t**) input;

	return thisInstance->renderCallback(inputBuffers, numChannel, frameCount);
}

int DeviceInputInstance::renderCallback(const float* const* samples, size_t numChannel, uint32_t nframes) {
	isPaRunning = true;

	for(size_t i = 0; i < numChannel; i++) {
		size_t dataSize;
		resamplingFilters[i].processSamples(resampledBuffer, samples[i], nframes);

		dataSize = resampledBuffer.size() * sizeof(resampledBuffer[0]);

		size_t availableData = jack_ringbuffer_write_space(ringBuffers[i].get());

		if(dataSize < availableData) {
			jack_ringbuffer_write(ringBuffers[i].get(), (const char*) resampledBuffer.data(), dataSize);
		} else {
			overflowOccured = true;
			overflowSize = availableData;
		}
	}

	return paContinue;
}

int DeviceInputInstance::postProcessSamples(float** samples, size_t numChannel, uint32_t nframes) {
	bool underflowOccured = false;
	size_t availableData = 0;

	if(!stream)
		return 0;

	for(size_t i = 0; i < ringBuffers.size(); i++) {
		availableData = jack_ringbuffer_read_space(ringBuffers[i].get());

		if(availableData >= nframes * sizeof(samples[i][0])) {
			jack_ringbuffer_read(ringBuffers[i].get(), (char*) samples[i], nframes * sizeof(samples[i][0]));
		} else {
			memset(reinterpret_cast<char*>(samples[i]), 0, nframes * sizeof(samples[i][0]));
			underflowOccured = true;
			underflowSize = availableData;
		}
	}

	if(underflowOccured) {
		underflowOccured = true;
		bufferLatencyHistory.clear();
	} else if(availableData) {
		bufferLatencyHistory.push_back(availableData / sizeof(jack_default_audio_sample_t));

		if(bufferLatencyHistory.size() == bufferLatencyMeasurePeriodSize) {
			double averageLatency = 0;
			for(auto value : bufferLatencyHistory) {
				averageLatency += value;
			}
			averageLatency /= bufferLatencyHistory.size();
			bufferLatencyHistory.clear();

			if(previousAverageLatency != 0) {
				double sampleLatencyDiffOver1s = averageLatency - previousAverageLatency;
				double sampleMeasureDuration = bufferLatencyMeasurePeriodSize * nframes;

				// Ignore drift of more than 0.1%
				//				if(sampleLatencyDiffOver1s > (-sampleMeasureDuration / 1000) &&
				//				   sampleLatencyDiffOver1s < (sampleMeasureDuration / 1000)) {
				clockDriftPpm = 1000000.0 * sampleLatencyDiffOver1s / sampleMeasureDuration;
				//				}
			}

			previousAverageLatency = averageLatency;
		}
	}

	return 0;
}

void DeviceInputInstance::onTimer() {
	if(overflowOccured) {
		printf("%s: Overflow: %d, %d\n", inputDevice.c_str(), bufferLatencyNr, overflowSize);
		overflowOccured = false;
	}
	if(underflowOccured) {
		printf("%s: underrun: %d, %d\n", inputDevice.c_str(), bufferLatencyNr, underflowSize);
		underflowOccured = false;
	}
	if(clockDriftPpm) {
		printf("%s: average latency: %f\n", inputDevice.c_str(), previousAverageLatency);
		printf("%s: drift: %f\n", inputDevice.c_str(), clockDriftPpm);
		clockDriftPpm = 0;
	}

	if(!isPaRunning) {
		printf("%s: portaudio not running !\n", inputDevice.c_str());
	} else {
		isPaRunning = false;
	}
}
