#ifndef DEVICEINPUTJACKINSTANCE_H
#define DEVICEINPUTJACKINSTANCE_H

#include "../json.h"
#include "IAudioEndpoint.h"
#include <stdint.h>
// Need to be after else stdint might conflict
#include <jack/jack.h>
#include <jack/ringbuffer.h>
#include <memory.h>
#include <vector>

#include "portaudio.h"

class DeviceInputInstance : public IAudioEndpoint {
public:
	static std::vector<std::string> getDeviceList();
	static int getDeviceIndex(const std::string& name);

	virtual const char* getName() override;
	virtual int start(int index, size_t numChannel, int sampleRate, int jackBufferSize) override;
	virtual void stop() override;
	virtual void setParameters(const nlohmann::json& json) override;
	virtual nlohmann::json getParameters() override;

	virtual int postProcessSamples(float** samples, size_t numChannel, uint32_t nframes) override;

protected:
	static int renderCallback(const void* input,
	                          void* output,
	                          unsigned long frameCount,
	                          const PaStreamCallbackTimeInfo* timeInfo,
	                          PaStreamCallbackFlags statusFlags,
	                          void* userData);

private:
	std::string inputDevice = "default_in";
	PaStream* stream = nullptr;
	std::vector<std::unique_ptr<jack_ringbuffer_t, void (*)(jack_ringbuffer_t*)>> ringBuffers;
};

#endif
