#ifndef REMOTEOUTPUTJACKINSTANCE_H
#define REMOTEOUTPUTJACKINSTANCE_H

#include "../json.h"
#include "IAudioEndpoint.h"
#include <stdint.h>
// Need to be after else stdint might conflict
#include <jack/jack.h>

#include "RemoteUdpOutput.h"
#include "ResamplingFilter.h"

class RemoteOutputInstance : public IAudioEndpoint {
public:
	RemoteOutputInstance();

	virtual const char* getName() override;
	virtual int start(int index, size_t numChannel, int sampleRate, int jackBufferSize) override;
	virtual void stop() override;
	virtual void setParameters(const nlohmann::json& json) override;
	virtual nlohmann::json getParameters() override;

	virtual int postProcessSamples(float** samples, size_t numChannel, uint32_t nframes) override;

private:
	RemoteUdpOutput remoteUdpOutput;
	std::vector<float> resampledBuffer[2];
	std::vector<float> outBuffers[2];
	std::vector<ResamplingFilter> resamplingFilters;
	std::string ip;
	int port;
	bool addVbanHeader = true;
};

#endif
