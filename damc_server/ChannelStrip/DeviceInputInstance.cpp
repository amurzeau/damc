#include "DeviceInputInstance.h"
#include <spdlog/spdlog.h>
#include <stdio.h>

#ifdef _WIN32
#include <pa_win_wasapi.h>
#endif

DeviceInputInstance::DeviceInputInstance(OscContainer* parent)
    : OscContainer(parent, "device"),
      stream(nullptr),
      oscDeviceName(this, "deviceName", "default_in"),
      oscClockDrift(this, "clockDrift", 0.0f),
      oscMeasuredClockDrift(this, "measuredClockDrift", 0.0f),
      oscBufferSize(this, "bufferSize", 0),
      oscActualBufferSize(this, "actualBufferSize", 0),
      oscDeviceSampleRate(this, "deviceSampleRate", 48000),
#ifdef _WIN32
      oscExclusiveMode(this, "exclusiveMode", true),
#endif
      oscIsRunning(this, "isRunning", false),
      oscUnderflowCount(this, "underflowCount", 0),
      oscOverflowCount(this, "overflowCount", 0),
      deviceSampleRateMeasure(this, "realSampleRate") {
	direction = D_Input;

	oscDeviceName.addCheckCallback([this](const std::string&) { return stream == nullptr; });
	oscBufferSize.addCheckCallback([this](int newValue) { return stream == nullptr && newValue >= 0; });
	oscDeviceSampleRate.addCheckCallback([this](int newValue) { return stream == nullptr && newValue > 0; });

#ifdef _WIN32
	oscExclusiveMode.addCheckCallback([this](int) { return stream == nullptr; });
#endif

	oscClockDrift.addChangeCallback([this](float newValue) {
		for(auto& resamplingFilter : resamplingFilters) {
			resamplingFilter.setClockDrift(newValue);
		}
	});
	oscDeviceSampleRate.addChangeCallback([this](float newValue) {
		for(auto& resamplingFilter : resamplingFilters) {
			resamplingFilter.setSourceSamplingRate(newValue);
		}
	});
}

DeviceInputInstance::~DeviceInputInstance() {
	DeviceInputInstance::stop();
}

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

static size_t getSimilarity(const char* str1, const char* str2) {
	size_t similarity = 0;
	size_t i;

	for(i = 0; str1[i] && str2[i]; i++) {
		if(str1[i] == str2[i]) {
			similarity++;
		}
	}

	// If both end at the same time, add a bonus 1
	if(str1[i] == str2[i]) {
		similarity++;
	}

	return similarity;
}

int DeviceInputInstance::getDeviceIndex(const std::string& name) {
	int numDevices = Pa_GetDeviceCount();

	if(name == "default_out") {
		return Pa_GetDefaultOutputDevice();
	} else if(name == "default_in") {
		return Pa_GetDefaultInputDevice();
	}

	size_t bestSimilarity = 0;
	size_t bestIndex = -1;

	for(int i = 0; i < numDevices; i++) {
		const PaDeviceInfo* deviceInfo = Pa_GetDeviceInfo(i);
		std::string devName;

		devName = std::string(Pa_GetHostApiInfo(deviceInfo->hostApi)->name) + "::" + std::string(deviceInfo->name);
		size_t similarity = getSimilarity(devName.c_str(), name.c_str());
		if(similarity > bestSimilarity) {
			bestSimilarity = similarity;
			bestIndex = i;
		}
	}

	return bestIndex;
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

	inputDeviceIndex = getDeviceIndex(oscDeviceName);

	if(inputDeviceIndex < 0 || inputDeviceIndex >= Pa_GetDeviceCount()) {
		SPDLOG_ERROR("Bad portaudio input device {}", oscDeviceName.get());
		return paInvalidDevice;
	}

	int32_t bufferSize = (oscBufferSize.get() > 0)
	                         ? oscBufferSize
	                         : Pa_GetDeviceInfo(inputDeviceIndex)->defaultLowInputLatency * oscDeviceSampleRate + 0.5;

	resampledBuffer.reserve(oscDeviceSampleRate);
	resamplingFilters.resize(numChannel);
	ringBuffers.clear();
	for(size_t i = 0; i < numChannel; i++) {
		std::unique_ptr<jack_ringbuffer_t, void (*)(jack_ringbuffer_t*)> buffer(nullptr, &jack_ringbuffer_free);
		buffer.reset(jack_ringbuffer_create((jackBufferSize + bufferSize * sampleRate / oscDeviceSampleRate) * 3 *
		                                    sizeof(jack_default_audio_sample_t)));
		ringBuffers.emplace_back(std::move(buffer));

		resamplingFilters[i].reset(oscDeviceSampleRate);
		resamplingFilters[i].setTargetSamplingRate(sampleRate);
		resamplingFilters[i].setClockDrift(this->oscClockDrift);
	}

	inputParameters.device = inputDeviceIndex;
	inputParameters.channelCount = numChannel;
	inputParameters.sampleFormat = paFloat32 | paNonInterleaved;  // 32 bit floating point output
	inputParameters.suggestedLatency = 1.0 * bufferSize / oscDeviceSampleRate;
	inputParameters.hostApiSpecificStreamInfo = NULL;

#ifdef _WIN32
	struct PaWasapiStreamInfo wasapiInfo = {};

	if(oscExclusiveMode && Pa_GetHostApiInfo(Pa_GetDeviceInfo(inputDeviceIndex)->hostApi)->type == paWASAPI) {
		wasapiInfo.size = sizeof(PaWasapiStreamInfo);
		wasapiInfo.hostApiType = paWASAPI;
		wasapiInfo.version = 1;
		wasapiInfo.flags = paWinWasapiExclusive;
		wasapiInfo.channelMask = 0;
		wasapiInfo.hostProcessorOutput = nullptr;
		wasapiInfo.hostProcessorInput = nullptr;
		wasapiInfo.threadPriority = eThreadPriorityNone;
		inputParameters.hostApiSpecificStreamInfo = &wasapiInfo;
		SPDLOG_INFO("Using exclusive mode");
	}
#endif

	int ret = Pa_OpenStream(&stream,
	                        &inputParameters,
	                        nullptr,
	                        oscDeviceSampleRate,
	                        paFramesPerBufferUnspecified,
	                        paClipOff | paDitherOff,
	                        &renderCallbackStatic,
	                        this);
	if(ret != paNoError) {
		SPDLOG_ERROR("Portaudio open error: {}({}), device: {}::{}",
		             Pa_GetErrorText(ret),
		             ret,
		             Pa_GetHostApiInfo(Pa_GetDeviceInfo(inputDeviceIndex)->hostApi)->name,
		             Pa_GetDeviceInfo(inputDeviceIndex)->name);
		return ret;
	}

	SPDLOG_INFO("Using output device {} {}, {} with latency {}",
	            inputDeviceIndex,
	            Pa_GetHostApiInfo(Pa_GetDeviceInfo(inputDeviceIndex)->hostApi)->name,
	            Pa_GetDeviceInfo(inputDeviceIndex)->name,
	            Pa_GetStreamInfo(stream)->inputLatency);

	bufferLatencyMeasurePeriodSize = 60 * sampleRate / jackBufferSize;
	bufferLatencyNr = 0;
	bufferLatencyHistory.clear();
	bufferLatencyHistory.reserve(bufferLatencyMeasurePeriodSize);
	previousAverageLatency = 0;
	clockDriftPpm = 0;
	isPaRunning = false;
	oscActualBufferSize = Pa_GetStreamInfo(stream)->inputLatency * oscDeviceSampleRate + 0.5;
	SPDLOG_INFO("Using buffer size {}, device sample rate: {}", oscActualBufferSize.get(), oscDeviceSampleRate.get());

	Pa_StartStream(stream);

	return ret;
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
	deviceSampleRateMeasure.notifySampleProcessed(nframes);

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
		this->underflowOccured = true;
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

void DeviceInputInstance::onFastTimer() {
	if(overflowOccured) {
		SPDLOG_DEBUG("{}: Overflow: {}, {}", oscDeviceName.get(), bufferLatencyNr, overflowSize);
		oscOverflowCount = oscOverflowCount + 1;
		overflowOccured = false;
	}
	if(underflowOccured) {
		SPDLOG_DEBUG("{}: underrun: {}, {}", oscDeviceName.get(), bufferLatencyNr, underflowSize);
		oscUnderflowCount = oscUnderflowCount + 1;
		underflowOccured = false;
	}
	if(clockDriftPpm) {
		SPDLOG_DEBUG("{}: average latency: {}", oscDeviceName.get(), previousAverageLatency);
		SPDLOG_DEBUG("{}: drift: {}", oscDeviceName.get(), clockDriftPpm);
		oscMeasuredClockDrift = clockDriftPpm;
		clockDriftPpm = 0;
	}
}

void DeviceInputInstance::onSlowTimer() {
	if(!isPaRunning) {
		SPDLOG_DEBUG("{}: portaudio not running !", oscDeviceName.get());
		oscIsRunning = false;
	} else {
		isPaRunning = false;
		oscIsRunning = true;
	}

	deviceSampleRateMeasure.onTimeoutTimer();
}
