#pragma once

#include "IAudioEndpoint.h"
#include <stdint.h>
// Need to be after else stdint might conflict
#include "ResamplingFilter.h"
#include <Osc/OscContainer.h>
#include <Osc/OscVariable.h>
#include <atomic>
#include <jack/jack.h>
#include <jack/ringbuffer.h>
#include <memory.h>
#include <vector>

#include <windows.h>

#include <MMSystem.h>
#include <audioclient.h>
#include <mmdeviceapi.h>

class WasapiInstance : public IAudioEndpoint, public OscContainer {
public:
	enum Format { F_Float, F_Int32, F_Int24, F_Int16 };

	static std::vector<std::string> getDeviceList();

	WasapiInstance(OscContainer* parent, Direction direction);
	~WasapiInstance();

	virtual const char* getName() override;
	virtual int start(int index, size_t numChannel, int sampleRate, int jackBufferSize) override;
	virtual void stop() override;
	virtual void onTimer() override;

	virtual int postProcessSamples(float** samples, size_t numChannel, uint32_t nframes) override;

private:
	uint32_t initializeWasapi(size_t numChannel, int sampleRate, int bufferSize);
	HRESULT findAudioConfig(IAudioClient* pAudioClient, size_t numChannel, WAVEFORMATEX** pFormat);
	void printAudioConfig(const WAVEFORMATEXTENSIBLE* pFormat);

	HRESULT getDeviceByName(std::string name, IMMDevice** ppMMDevice);

	int postProcessSamplesRender(float** samples, size_t numChannel, uint32_t nframes);
	int postProcessSamplesCapture(float** samples, size_t numChannel, uint32_t nframes);

	void copySamplesRender(
	    Format format, float** input, size_t numFrameToRead, void* output, size_t numChannel, uint32_t nframes);
	void copySamplesCapture(
	    Format format, void* input, size_t numFrameToRead, float** output, size_t numChannel, uint32_t nframes);

private:
	IMMDevice* pDevice = nullptr;
	IAudioClient* pAudioClient = nullptr;
	IAudioRenderClient* pRenderClient = nullptr;
	IAudioCaptureClient* pCaptureClient = nullptr;
	HANDLE audioEvent;

	UINT32 wasapiBufferSize;

	std::vector<ResamplingFilter> resamplingFilters;
	std::vector<std::vector<float>> resampledBuffer;
	int jackSampleRate;
	Format sampleFormat = F_Int32;
	bool overflowOccured;
	bool underflowOccured;

	OscVariable<std::string> oscDeviceName;
	OscVariable<int> oscDeviceSampleRate;
	OscVariable<float> oscClockDrift;
	OscVariable<bool> oscExclusiveMode;

	uint32_t bufferLatencyNr;
	std::vector<uint32_t> bufferLatencyHistory;
	size_t bufferLatencyMeasurePeriodSize;
	double previousAverageLatency;
	double clockDriftPpm;

	size_t underflowSize;
	size_t overflowSize;

	size_t maxBufferSize;
};
