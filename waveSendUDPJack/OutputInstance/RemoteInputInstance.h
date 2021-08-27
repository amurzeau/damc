#ifndef REMOTEINPUTJACKINSTANCE_H
#define REMOTEINPUTJACKINSTANCE_H

#include "IAudioEndpoint.h"
#include <stdint.h>
// Need to be after else stdint might conflict
#include <jack/jack.h>

#include "RemoteUdpInput.h"
#include "ResamplingFilter.h"

class RemoteInputInstance : public IAudioEndpoint, public OscContainer {
public:
	RemoteInputInstance(OscContainer* parent);

	virtual const char* getName() override;
	virtual int start(int index, size_t numChannel, int sampleRate, int jackBufferSize) override;
	virtual void stop() override;

	virtual int postProcessSamples(float** samples, size_t numChannel, uint32_t nframes) override;

private:
	RemoteUdpInput remoteUdpInput;
	double sampleRate;
	std::vector<float> resampledBuffer[2];
	std::vector<float> inBuffers[2];
	std::vector<ResamplingFilter> resamplingFilters;

	OscVariable<std::string> oscIp;
	OscVariable<int> oscPort;
	OscVariable<float> oscClockDrift;
};

#endif
