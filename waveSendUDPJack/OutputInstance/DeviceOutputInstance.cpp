#include "DeviceOutputInstance.h"
#include <stdio.h>

std::vector<std::string> DeviceOutputInstance::getDeviceList() {
	std::vector<std::string> result;
	int numDevices = Pa_GetDeviceCount();

	for(int i = 0; i < numDevices; i++) {
		const PaDeviceInfo* deviceInfo = Pa_GetDeviceInfo(i);
		std::string name;

		/*if(deviceInfo->maxOutputChannels <= 0)
		    continue;*/

		name = std::string(Pa_GetHostApiInfo(deviceInfo->hostApi)->name) + "::" + std::string(deviceInfo->name);
		result.push_back(name);
	}

	return result;
}

int DeviceOutputInstance::getDeviceIndex(const std::string& name) {
	int numDevices = Pa_GetDeviceCount();

	if(name == "default_out") {
		return Pa_GetDefaultOutputDevice();
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

void DeviceOutputInstance::stop() {
	if(stream) {
		Pa_StopStream(stream);
		Pa_CloseStream(stream);
	}
	stream = nullptr;
}

const char* DeviceOutputInstance::getName() {
	return "device";
}

int DeviceOutputInstance::start(int index, size_t numChannel, int sampleRate, int jackBufferSize) {
	PaStreamParameters outputParameters;
	int outputDeviceIndex;

	outputDeviceIndex = getDeviceIndex(outputDevice);

	doDebug = outputDeviceIndex == 24;

	if(outputDeviceIndex < 0 || outputDeviceIndex >= Pa_GetDeviceCount()) {
		printf("Bad portaudio output device %s\n", outputDevice.c_str());
		return paInvalidDevice;
	}

	resampledBuffer.reserve(sampleRate);
	resamplingFilters.resize(numChannel);
	for(size_t i = 0; i < numChannel; i++) {
		std::unique_ptr<jack_ringbuffer_t, void (*)(jack_ringbuffer_t*)> buffer(nullptr, &jack_ringbuffer_free);
		buffer.reset(jack_ringbuffer_create(sampleRate));
		ringBuffers.emplace_back(std::move(buffer));

		resamplingFilters[i].reset();
		resamplingFilters[i].setClockDrift(this->clockDrift);
	}

	outputParameters.device = outputDeviceIndex;
	outputParameters.channelCount = numChannel;
	outputParameters.sampleFormat = paFloat32 | paNonInterleaved;  // 32 bit floating point output
	outputParameters.suggestedLatency = jackBufferSize * 3 / 48000.0;
	outputParameters.hostApiSpecificStreamInfo = NULL;

	int ret = Pa_OpenStream(&stream,
	                        0,
	                        &outputParameters,
	                        sampleRate,
	                        paFramesPerBufferUnspecified,
	                        paNoFlag,  // Clipping is on...
	                        &renderCallback,
	                        this);
	if(ret != paNoError) {
		printf("Portaudio open error: %d, device: %s::%s\n",
		       ret,
		       Pa_GetHostApiInfo(Pa_GetDeviceInfo(outputDeviceIndex)->hostApi)->name,
		       Pa_GetDeviceInfo(outputDeviceIndex)->name);
	} else {
		printf("Using output device %d %s, %s with latency %.3f\n",
		       outputDeviceIndex,
		       Pa_GetHostApiInfo(Pa_GetDeviceInfo(outputDeviceIndex)->hostApi)->name,
		       Pa_GetDeviceInfo(outputDeviceIndex)->name,
		       Pa_GetStreamInfo(stream)->outputLatency);
	}

	Pa_StartStream(stream);

	return ret;
}

void DeviceOutputInstance::setParameters(const nlohmann::json& json) {
	outputDevice = json.value("device", outputDevice);

	auto clockDrift = json.find("clockDrift");
	if(clockDrift != json.end()) {
		this->clockDrift = clockDrift.value().get<float>();
		for(auto& resamplingFilter : resamplingFilters)
			resamplingFilter.setClockDrift(this->clockDrift);
	}
}

nlohmann::json DeviceOutputInstance::getParameters() {
	return nlohmann::json::object({{"device", outputDevice}, {"clockDrift", clockDrift}});
}

int DeviceOutputInstance::postProcessSamples(float** samples, size_t numChannel, uint32_t nframes) {
	if(!stream)
		return 0;

	for(size_t i = 0; i < numChannel; i++) {
		size_t dataSize;
		resamplingFilters[i].processSamples(resampledBuffer, samples[i], nframes);

		dataSize = resampledBuffer.size() * sizeof(resampledBuffer[0]);

		if(dataSize < jack_ringbuffer_write_space(ringBuffers[i].get())) {
			jack_ringbuffer_write(ringBuffers[i].get(), (const char*) resampledBuffer.data(), dataSize);
		} else {
			overflowOccured = true;
		}
	}

	//	jack_nframes_t current_frames;
	//	jack_time_t current_usecs;
	//	jack_time_t next_usecs;
	//	float period_usecs;
	//	jack_nframes_t call_frames;
	//	jack_time_t call_time_time;
	//	getLiveFrameTime(&current_frames, &current_usecs, &next_usecs, &period_usecs, &call_frames, &call_time_time);

	//	long available = Pa_GetStreamWriteAvailable(stream);
	//	currentJackTime = (call_time_time - current_usecs) / 1000000.0;

	//	const PaStreamInfo* streamInfo = Pa_GetStreamInfo(stream);

	//	currentPaTime =
	//	    currentJackTime + streamInfo->outputLatency - available / 48000.0 / resamplingFilters[0].getClockDrift();

	return 0;
}

int DeviceOutputInstance::renderCallback(const void* input,
                                         void* output,
                                         unsigned long frameCount,
                                         const PaStreamCallbackTimeInfo* timeInfo,
                                         PaStreamCallbackFlags statusFlags,
                                         void* userData) {
	DeviceOutputInstance* thisInstance = (DeviceOutputInstance*) userData;

	jack_default_audio_sample_t** outputBuffers = (jack_default_audio_sample_t**) output;

	for(size_t i = 0; i < thisInstance->ringBuffers.size(); i++) {
		size_t readData = jack_ringbuffer_read(
		    thisInstance->ringBuffers[i].get(), (char*) outputBuffers[i], frameCount * sizeof(outputBuffers[i][0]));
		if(readData < frameCount * sizeof(outputBuffers[i][0])) {
			memset(reinterpret_cast<char*>(outputBuffers[i]) + readData,
			       0,
			       frameCount * sizeof(outputBuffers[i][0]) - readData);
			thisInstance->underflowOccured = true;
		}
	}

	return paContinue;
}

void DeviceOutputInstance::onTimer() {
	//	static unsigned int counter;

	//	if((counter % 30) == 0 && doDebug)
	//		printf("%.6f, %.6f, %.6f, j: %.6f, b: %.6f\n",
	//		       currentJackTime - currentPaTime,
	//		       currentPaTime,
	//		       currentJackTime,
	//		       nextBufferedSampleJackTime / 48000.0,
	//		       jack_ringbuffer_read_space(ringBuffers[0].get()) / sizeof(float) / 48000.0);
	//	counter++;

	if(overflowOccured) {
		printf("%s: Overflow\n", outputDevice.c_str());
		overflowOccured = false;
	}
	if(underflowOccured) {
		printf("%s: underrun\n", outputDevice.c_str());
		underflowOccured = false;
	}
}
