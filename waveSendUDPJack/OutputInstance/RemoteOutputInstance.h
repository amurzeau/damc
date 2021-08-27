#ifndef REMOTEOUTPUTJACKINSTANCE_H
#define REMOTEOUTPUTJACKINSTANCE_H

#include "IAudioEndpoint.h"
#include <stdint.h>
// Need to be after else stdint might conflict
#include <jack/jack.h>

#include "RemoteUdpOutput.h"
#include "ResamplingFilter.h"

class RemoteOutputInstance : public IAudioEndpoint, public OscContainer {
public:
	RemoteOutputInstance(OscContainer* parent);

	virtual const char* getName() override;
	virtual int start(int index, size_t numChannel, int sampleRate, int jackBufferSize) override;
	virtual void stop() override;

	virtual int postProcessSamples(float** samples, size_t numChannel, uint32_t nframes) override;

private:
	RemoteUdpOutput remoteUdpOutput;
	std::vector<float> resampledBuffer[2];
	std::vector<float> outBuffers[2];
	std::vector<ResamplingFilter> resamplingFilters;

	OscVariable<std::string> oscIp;
	OscVariable<int> oscPort;
	OscVariable<float> oscClockDrift;
	OscVariable<bool> oscAddVbanHeader;
};

#endif
