#ifndef DEVICEINPUTJACKINSTANCE_H
#define DEVICEINPUTJACKINSTANCE_H

#include "IAudioEndpoint.h"
#include <stdint.h>
// Need to be after else stdint might conflict
#include "ResamplingFilter.h"
#include <Osc/OscContainer.h>
#include <Osc/OscVariable.h>
#include <jack/jack.h>
#include <jack/ringbuffer.h>
#include <memory.h>
#include <vector>

#include "portaudio.h"

class DeviceInputInstance : public IAudioEndpoint, public OscContainer {
public:
	DeviceInputInstance(OscContainer* parent);
	~DeviceInputInstance();

	static std::vector<std::string> getDeviceList();
	static int getDeviceIndex(const std::string& name);

	virtual const char* getName() override;
	virtual int start(int index, size_t numChannel, int sampleRate, int jackBufferSize) override;
	virtual void stop() override;
	virtual void onTimer() override;

	virtual int postProcessSamples(float** samples, size_t numChannel, uint32_t nframes) override;

protected:
	static int renderCallbackStatic(const void* input,
	                                void* output,
	                                unsigned long frameCount,
	                                const PaStreamCallbackTimeInfo* timeInfo,
	                                PaStreamCallbackFlags statusFlags,
	                                void* userData);
	int renderCallback(const float* const* samples, size_t numChannel, uint32_t nframes);

private:
	PaStream* stream;
	std::vector<std::unique_ptr<jack_ringbuffer_t, void (*)(jack_ringbuffer_t*)>> ringBuffers;
	std::vector<ResamplingFilter> resamplingFilters;
	std::vector<float> resampledBuffer;
	bool overflowOccured = false;
	bool underflowOccured = false;

	OscVariable<std::string> oscDeviceName;
	OscVariable<float> oscClockDrift;
	OscVariable<int32_t> oscDeviceSampleRate;
	OscVariable<bool> oscExclusiveMode;

	uint32_t bufferLatencyNr = 0;
	std::vector<uint32_t> bufferLatencyHistory;
	size_t bufferLatencyMeasurePeriodSize = 0;
	double previousAverageLatency = 0;
	double clockDriftPpm = 0;

	bool isPaRunning = false;
	size_t underflowSize = 0;
	size_t overflowSize = 0;
};

#endif
