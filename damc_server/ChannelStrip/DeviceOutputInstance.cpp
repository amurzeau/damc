#include "DeviceOutputInstance.h"
#include <algorithm>
#include <spdlog/spdlog.h>
#include <stdio.h>

#ifdef _WIN32
#include <pa_win_wasapi.h>
#endif

DeviceOutputInstance::DeviceOutputInstance(OscContainer* parent)
    : OscContainer(parent, "device"),
      stream(nullptr),
      oscDeviceName(this, "deviceName", "default_out"),
      oscClockDrift(this, "clockDrift", 0.0f),
      oscMeasuredClockDrift(this, "measuredClockDrift", 0.0f),
      oscBufferSize(this, "bufferSize", 0),
      oscActualBufferSize(this, "actualBufferSize", 0),
      oscDeviceSampleRate(this, "deviceSampleRate", 48000),
#ifdef _WIN32
      oscExclusiveMode(this, "exclusiveMode", true),
#endif
      oscIsRunning(this, "isRunning", false),
      oscUnderflowCount(this, "underflowCount", 0, false),
      oscOverflowCount(this, "overflowCount", 0, false),
      deviceSampleRateMeasure(this, "realSampleRate") {
	oscDeviceName.addCheckCallback([this](const std::string&) { return stream == nullptr; });
	oscBufferSize.addCheckCallback([this](int newValue) { return stream == nullptr && newValue >= 0; });
	oscDeviceSampleRate.addCheckCallback([this](int newValue) { return stream == nullptr && newValue > 0; });
	oscDeviceSampleRate.addCheckCallback([this](int) { return stream == nullptr; });

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
			resamplingFilter.setTargetSamplingRate(newValue);
		}
	});
}

DeviceOutputInstance::~DeviceOutputInstance() {
	DeviceOutputInstance::stop();
}

std::vector<std::string> DeviceOutputInstance::getDeviceList() {
#ifdef _WIN32
	PaWasapi_UpdateDeviceList();
#endif
	std::vector<std::string> result;
	int numDevices = Pa_GetDeviceCount();

	result.push_back("default_in");
	result.push_back("default_out");

	for(int i = 0; i < numDevices; i++) {
		const PaDeviceInfo* deviceInfo = Pa_GetDeviceInfo(i);
		std::string name;

		if(deviceInfo->name[0] == 0)
			continue;

		name = std::string(Pa_GetHostApiInfo(deviceInfo->hostApi)->name) + "::" + std::string(deviceInfo->name);
		result.push_back(name);
	}

	if(numDevices > 0)
		std::sort(result.begin() + 2, result.end());

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

int DeviceOutputInstance::getDeviceIndex(const std::string& name) {
	int numDevices = Pa_GetDeviceCount();

	if(name == "default_out") {
		return Pa_GetDefaultOutputDevice();
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

	stream = nullptr;

	outputDeviceIndex = getDeviceIndex(oscDeviceName);

	if(outputDeviceIndex < 0 || outputDeviceIndex >= Pa_GetDeviceCount()) {
		SPDLOG_ERROR("Bad portaudio output device {}", oscDeviceName.get());
		return paInvalidDevice;
	}

	int32_t bufferSize = (oscBufferSize.get() > 0)
	                         ? oscBufferSize
	                         : Pa_GetDeviceInfo(outputDeviceIndex)->defaultLowOutputLatency * oscDeviceSampleRate + 0.5;

	resampledBuffer.reserve(sampleRate);
	resamplingFilters.resize(numChannel);
	ringBuffers.clear();
	for(size_t i = 0; i < numChannel; i++) {
		std::unique_ptr<jack_ringbuffer_t, void (*)(jack_ringbuffer_t*)> buffer(nullptr, &jack_ringbuffer_free);
		buffer.reset(jack_ringbuffer_create((jackBufferSize + bufferSize * sampleRate / oscDeviceSampleRate) * 3 *
		                                    sizeof(jack_default_audio_sample_t)));
		ringBuffers.emplace_back(std::move(buffer));

		resamplingFilters[i].reset(sampleRate);
		resamplingFilters[i].setTargetSamplingRate(oscDeviceSampleRate);
		resamplingFilters[i].setClockDrift(oscClockDrift);
	}

	outputParameters.device = outputDeviceIndex;
	outputParameters.channelCount = numChannel;
	outputParameters.sampleFormat = paFloat32 | paNonInterleaved;  // 32 bit floating point output
	outputParameters.suggestedLatency = 1.0 * bufferSize / oscDeviceSampleRate;
	outputParameters.hostApiSpecificStreamInfo = NULL;

#ifdef _WIN32
	struct PaWasapiStreamInfo wasapiInfo = {};

	if(oscExclusiveMode && Pa_GetHostApiInfo(Pa_GetDeviceInfo(outputDeviceIndex)->hostApi)->type == paWASAPI) {
		wasapiInfo.size = sizeof(PaWasapiStreamInfo);
		wasapiInfo.hostApiType = paWASAPI;
		wasapiInfo.version = 1;
		wasapiInfo.flags = paWinWasapiExclusive;
		wasapiInfo.channelMask = 0;
		wasapiInfo.hostProcessorOutput = nullptr;
		wasapiInfo.hostProcessorInput = nullptr;
		wasapiInfo.threadPriority = eThreadPriorityNone;
		outputParameters.hostApiSpecificStreamInfo = &wasapiInfo;
		SPDLOG_INFO("Using exclusive mode\n");
	}
#endif

	int ret = Pa_OpenStream(&stream,
	                        nullptr,
	                        &outputParameters,
	                        oscDeviceSampleRate,
	                        paFramesPerBufferUnspecified,
	                        paClipOff | paDitherOff,
	                        &renderCallback,
	                        this);
	if(ret != paNoError) {
		SPDLOG_ERROR("Portaudio open error: {}({}), device: {}::{}",
		             Pa_GetErrorText(ret),
		             ret,
		             Pa_GetHostApiInfo(Pa_GetDeviceInfo(outputDeviceIndex)->hostApi)->name,
		             Pa_GetDeviceInfo(outputDeviceIndex)->name);
		return ret;
	}

	SPDLOG_INFO("Using output device {} {}, {} with latency {}",
	            outputDeviceIndex,
	            Pa_GetHostApiInfo(Pa_GetDeviceInfo(outputDeviceIndex)->hostApi)->name,
	            Pa_GetDeviceInfo(outputDeviceIndex)->name,
	            Pa_GetStreamInfo(stream)->outputLatency);

	bufferLatencyMeasurePeriodSize = 60 * sampleRate / jackBufferSize;
	bufferLatencyNr = 0;
	bufferLatencyHistory.clear();
	bufferLatencyHistory.reserve(bufferLatencyMeasurePeriodSize);
	previousAverageLatency = 0;
	clockDriftPpm = 0;
	isPaRunning = false;
	oscActualBufferSize = Pa_GetStreamInfo(stream)->outputLatency * oscDeviceSampleRate + 0.5;
	SPDLOG_INFO("Using buffer size {}, device sample rate: {}", oscActualBufferSize.get(), oscDeviceSampleRate.get());

	Pa_StartStream(stream);

	return ret;
}

int DeviceOutputInstance::postProcessSamples(float** samples, size_t numChannel, uint32_t nframes) {
	if(!stream)
		return 0;

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

	bool underflowOccured = false;
	size_t availableData = 0;

	thisInstance->isPaRunning = true;
	thisInstance->deviceSampleRateMeasure.notifySampleProcessed(frameCount);

	for(size_t i = 0; i < thisInstance->ringBuffers.size(); i++) {
		availableData = jack_ringbuffer_read_space(thisInstance->ringBuffers[i].get());

		if(availableData >= frameCount * sizeof(outputBuffers[i][0])) {
			jack_ringbuffer_read(
			    thisInstance->ringBuffers[i].get(), (char*) outputBuffers[i], frameCount * sizeof(outputBuffers[i][0]));
		} else {
			memset(reinterpret_cast<char*>(outputBuffers[i]), 0, frameCount * sizeof(outputBuffers[i][0]));
			underflowOccured = true;
			thisInstance->underflowSize = availableData;
		}
	}

	if(underflowOccured) {
		thisInstance->underflowOccured = true;
		thisInstance->bufferLatencyHistory.clear();
	} else if(availableData) {
		thisInstance->bufferLatencyHistory.push_back(availableData / sizeof(jack_default_audio_sample_t));

		if(thisInstance->bufferLatencyHistory.size() == thisInstance->bufferLatencyMeasurePeriodSize) {
			double averageLatency = 0;
			for(auto value : thisInstance->bufferLatencyHistory) {
				averageLatency += value;
			}
			averageLatency /= thisInstance->bufferLatencyHistory.size();
			thisInstance->bufferLatencyHistory.clear();

			if(thisInstance->previousAverageLatency != 0) {
				double sampleLatencyDiffOver1s = averageLatency - thisInstance->previousAverageLatency;
				double sampleMeasureDuration = thisInstance->bufferLatencyMeasurePeriodSize * frameCount;

				// Ignore drift of more than 0.1%
				//				if(sampleLatencyDiffOver1s > (-sampleMeasureDuration / 1000) &&
				//				   sampleLatencyDiffOver1s < (sampleMeasureDuration / 1000)) {
				thisInstance->clockDriftPpm = 1000000.0 * sampleLatencyDiffOver1s / sampleMeasureDuration;
				//				}
			}

			thisInstance->previousAverageLatency = averageLatency;
		}
	}

	return paContinue;
}

void DeviceOutputInstance::onFastTimer() {
	if(clockDriftPpm) {
		SPDLOG_DEBUG("{}: average latency: {}", oscDeviceName.get(), previousAverageLatency);
		SPDLOG_DEBUG("{}: drift: {}", oscDeviceName.get(), clockDriftPpm);
		oscMeasuredClockDrift = clockDriftPpm;
		clockDriftPpm = 0;
	}
}

void DeviceOutputInstance::onSlowTimer() {
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
	if(!isPaRunning) {
		SPDLOG_DEBUG("{}: portaudio not running !", oscDeviceName.get());
		oscIsRunning = false;
	} else {
		isPaRunning = false;
		oscIsRunning = true;
	}

	deviceSampleRateMeasure.onTimeoutTimer();
}
