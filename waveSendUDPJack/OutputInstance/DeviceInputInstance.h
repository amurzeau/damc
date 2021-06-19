#ifndef DEVICEINPUTJACKINSTANCE_H
#define DEVICEINPUTJACKINSTANCE_H

#include "../json.h"
#include "IAudioEndpoint.h"
#include <stdint.h>
// Need to be after else stdint might conflict
#include "ResamplingFilter.h"
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
	std::string inputDevice = "default_in";
	PaStream* stream = nullptr;
	std::vector<std::unique_ptr<jack_ringbuffer_t, void (*)(jack_ringbuffer_t*)>> ringBuffers;
	std::vector<ResamplingFilter> resamplingFilters;
	std::vector<float> resampledBuffer;
	float clockDrift = 1.0f;
	float deviceSampleRate = 48000.0f;
	bool overflowOccured;
	bool underflowOccured;

	uint32_t bufferLatencyNr;
	std::vector<uint32_t> bufferLatencyHistory;
	size_t bufferLatencyMeasurePeriodSize;
	double previousAverageLatency;
	double clockDriftPpm;

	bool isPaRunning;
	size_t underflowSize;
	size_t overflowSize;
};

#endif
