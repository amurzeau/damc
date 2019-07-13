#ifndef REMOTEINPUTJACKINSTANCE_H
#define REMOTEINPUTJACKINSTANCE_H

#include "../json.h"
#include "IAudioEndpoint.h"
#include <stdint.h>
// Need to be after else stdint might conflict
#include <jack/jack.h>

#include "RemoteUdpInput.h"
#include "ResamplingFilter.h"

class RemoteInputInstance : public IAudioEndpoint {
public:
	virtual const char* getName() override;
	virtual int start(int index, size_t numChannel, int sampleRate, int jackBufferSize) override;
	virtual void stop() override;
	virtual void setParameters(const nlohmann::json& json) override;
	virtual nlohmann::json getParameters() override;

	virtual int postProcessSamples(float** samples, size_t numChannel, uint32_t nframes) override;

private:
	RemoteUdpInput remoteUdpInput;
	std::vector<float> resampledBuffer[2];
	std::vector<float> inBuffers[2];
	std::vector<ResamplingFilter> resamplingFilters;
	std::string ip = "127.0.0.1";
	int port = 2305;
	float clockDrift = 1.0f;
};

#endif
