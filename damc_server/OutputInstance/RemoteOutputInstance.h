#pragma once

#include "IAudioEndpoint.h"
#include <stdint.h>
// Need to be after else stdint might conflict
#include <jack/jack.h>

#include "RemoteUdpOutput.h"
#include "ResamplingFilter.h"
#include <Osc/OscContainer.h>
#include <Osc/OscVariable.h>

class RemoteOutputInstance : public IAudioEndpoint, public OscContainer {
public:
	RemoteOutputInstance(OscContainer* parent);
	~RemoteOutputInstance();

	virtual const char* getName() override;
	virtual int start(int index, size_t numChannel, int sampleRate, int jackBufferSize) override;
	virtual void stop() override;
	virtual void onSlowTimer() override;

	virtual int postProcessSamples(float** samples, size_t numChannel, uint32_t nframes) override;

private:
	RemoteUdpOutput remoteUdpOutput;
	std::vector<float> resampledBuffer[2];
	std::vector<float> outBuffers[2];
	std::vector<ResamplingFilter> resamplingFilters;

	OscVariable<std::string> oscIp;
	OscVariable<int> oscPort;
	OscVariable<int32_t> oscDeviceSampleRate;
	OscVariable<float> oscClockDrift;
	OscVariable<bool> oscAddVbanHeader;
};
