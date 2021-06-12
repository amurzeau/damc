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
		inputDeviceIndex = -1;
		return paInvalidDevice;
	}

	inputParameters.device = inputDeviceIndex;
	inputParameters.channelCount = numChannel;
	inputParameters.sampleFormat = paFloat32 | paNonInterleaved;  // 32 bit floating point output
	inputParameters.suggestedLatency = Pa_GetDeviceInfo(inputParameters.device)->defaultLowInputLatency;
	inputParameters.hostApiSpecificStreamInfo = NULL;

	for(size_t i = 0; i < numChannel; i++) {
		std::unique_ptr<jack_ringbuffer_t, void (*)(jack_ringbuffer_t*)> buffer(nullptr, &jack_ringbuffer_free);
		buffer.reset(jack_ringbuffer_create(jackBufferSize * 5));
		ringBuffers.emplace_back(std::move(buffer));
	}

	int ret = Pa_OpenStream(&stream,
	                        &inputParameters,
	                        NULL,
	                        sampleRate,
	                        paFramesPerBufferUnspecified,
	                        paClipOff | paDitherOff,  // Clipping is on...
	                        &renderCallback,
	                        this);
	if(ret != paNoError) {
		printf("Portaudio open error: %d\n", ret);
		return ret;
	}

	printf("Using in device %s, %s\n",
	       Pa_GetHostApiInfo(Pa_GetDeviceInfo(inputDeviceIndex)->hostApi)->name,
	       Pa_GetDeviceInfo(inputDeviceIndex)->name);

	Pa_StartStream(stream);

	return ret;
}

void DeviceInputInstance::setParameters(const nlohmann::json& json) {
	inputDevice = json.value("device", inputDevice);
}

nlohmann::json DeviceInputInstance::getParameters() {
	return nlohmann::json::object({{"device", inputDevice}});
}

int DeviceInputInstance::postProcessSamples(float** samples, size_t numChannel, uint32_t nframes) {
	for(size_t i = 0; i < ringBuffers.size(); i++) {
		jack_ringbuffer_read(ringBuffers[i].get(), (char*) samples[i], nframes * sizeof(samples[i][0]));
	}

	return 0;
}

int DeviceInputInstance::renderCallback(const void* input,
                                        void* output,
                                        unsigned long frameCount,
                                        const PaStreamCallbackTimeInfo* timeInfo,
                                        PaStreamCallbackFlags statusFlags,
                                        void* userData) {
	DeviceInputInstance* thisInstance = (DeviceInputInstance*) userData;
	size_t numChannel = thisInstance->ringBuffers.size();

	const jack_default_audio_sample_t** inputBuffers = (const jack_default_audio_sample_t**) input;

	for(size_t i = 0; i < numChannel; i++) {
		if(jack_ringbuffer_write_space(thisInstance->ringBuffers[i].get()) < frameCount * sizeof(inputBuffers[i][0]))
			return 1;
	}

	for(size_t i = 0; i < numChannel; i++) {
		jack_ringbuffer_write(
		    thisInstance->ringBuffers[i].get(), (const char*) inputBuffers[i], frameCount * sizeof(inputBuffers[i][0]));
	}

	return paContinue;
}
