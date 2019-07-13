#ifndef AUDIOENDPOINT_H
#define AUDIOENDPOINT_H

#include "../Filter/FilteringChain.h"
#include "../json.h"
#include <stdint.h>
#include <uv.h>

class ControlServer;
class ControlClient;

class IAudioEndpoint {
public:
	virtual ~IAudioEndpoint() {}
	virtual const char* getName() = 0;
	virtual int start(int index, size_t numChannel, int sampleRate, int jackBufferSize) { return 0; }
	virtual void stop() {}
	virtual void setParameters(const nlohmann::json& json) {}
	virtual nlohmann::json getParameters() { return nlohmann::json(); }
	virtual void onTimer() {}

	virtual int postProcessSamples(float** samples, size_t numChannel, uint32_t nframes) = 0;
};

#endif
