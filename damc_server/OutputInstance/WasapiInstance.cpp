#include <initguid.h>

#include "WasapiInstance.h"
#include <FunctionDiscoveryKeys_devpkey.h>
#include <algorithm>
#include <codecvt>
#include <ksmedia.h>
#include <locale>
#include <spdlog/spdlog.h>
#include <stdio.h>
#include <string>

#define REFTIMES_PER_SEC 10000000
#define REFTIMES_PER_MILLISEC 10000

/* Speaker Positions: */
#define PAWIN_SPEAKER_FRONT_LEFT (0x1)
#define PAWIN_SPEAKER_FRONT_RIGHT (0x2)
#define PAWIN_SPEAKER_FRONT_CENTER (0x4)
#define PAWIN_SPEAKER_LOW_FREQUENCY (0x8)
#define PAWIN_SPEAKER_BACK_LEFT (0x10)
#define PAWIN_SPEAKER_BACK_RIGHT (0x20)
#define PAWIN_SPEAKER_FRONT_LEFT_OF_CENTER (0x40)
#define PAWIN_SPEAKER_FRONT_RIGHT_OF_CENTER (0x80)
#define PAWIN_SPEAKER_BACK_CENTER (0x100)
#define PAWIN_SPEAKER_SIDE_LEFT (0x200)
#define PAWIN_SPEAKER_SIDE_RIGHT (0x400)
#define PAWIN_SPEAKER_TOP_CENTER (0x800)
#define PAWIN_SPEAKER_TOP_FRONT_LEFT (0x1000)
#define PAWIN_SPEAKER_TOP_FRONT_CENTER (0x2000)
#define PAWIN_SPEAKER_TOP_FRONT_RIGHT (0x4000)
#define PAWIN_SPEAKER_TOP_BACK_LEFT (0x8000)
#define PAWIN_SPEAKER_TOP_BACK_CENTER (0x10000)
#define PAWIN_SPEAKER_TOP_BACK_RIGHT (0x20000)

/* Bit mask locations reserved for future use */
#define PAWIN_SPEAKER_RESERVED (0x7FFC0000)

/* Used to specify that any possible permutation of speaker configurations */
#define PAWIN_SPEAKER_ALL (0x80000000)

#define PAWIN_SPEAKER_DIRECTOUT 0
#define PAWIN_SPEAKER_MONO (PAWIN_SPEAKER_FRONT_CENTER)
#define PAWIN_SPEAKER_STEREO (PAWIN_SPEAKER_FRONT_LEFT | PAWIN_SPEAKER_FRONT_RIGHT)
#define PAWIN_SPEAKER_QUAD \
	(PAWIN_SPEAKER_FRONT_LEFT | PAWIN_SPEAKER_FRONT_RIGHT | PAWIN_SPEAKER_BACK_LEFT | PAWIN_SPEAKER_BACK_RIGHT)
#define PAWIN_SPEAKER_SURROUND \
	(PAWIN_SPEAKER_FRONT_LEFT | PAWIN_SPEAKER_FRONT_RIGHT | PAWIN_SPEAKER_FRONT_CENTER | PAWIN_SPEAKER_BACK_CENTER)
#define PAWIN_SPEAKER_5POINT1 \
	(PAWIN_SPEAKER_FRONT_LEFT | PAWIN_SPEAKER_FRONT_RIGHT | PAWIN_SPEAKER_FRONT_CENTER | PAWIN_SPEAKER_LOW_FREQUENCY | \
	 PAWIN_SPEAKER_BACK_LEFT | PAWIN_SPEAKER_BACK_RIGHT)
#define PAWIN_SPEAKER_7POINT1 \
	(PAWIN_SPEAKER_FRONT_LEFT | PAWIN_SPEAKER_FRONT_RIGHT | PAWIN_SPEAKER_FRONT_CENTER | PAWIN_SPEAKER_LOW_FREQUENCY | \
	 PAWIN_SPEAKER_BACK_LEFT | PAWIN_SPEAKER_BACK_RIGHT | PAWIN_SPEAKER_FRONT_LEFT_OF_CENTER | \
	 PAWIN_SPEAKER_FRONT_RIGHT_OF_CENTER)
#define PAWIN_SPEAKER_5POINT1_SURROUND \
	(PAWIN_SPEAKER_FRONT_LEFT | PAWIN_SPEAKER_FRONT_RIGHT | PAWIN_SPEAKER_FRONT_CENTER | PAWIN_SPEAKER_LOW_FREQUENCY | \
	 PAWIN_SPEAKER_SIDE_LEFT | PAWIN_SPEAKER_SIDE_RIGHT)
#define PAWIN_SPEAKER_7POINT1_SURROUND \
	(PAWIN_SPEAKER_FRONT_LEFT | PAWIN_SPEAKER_FRONT_RIGHT | PAWIN_SPEAKER_FRONT_CENTER | PAWIN_SPEAKER_LOW_FREQUENCY | \
	 PAWIN_SPEAKER_BACK_LEFT | PAWIN_SPEAKER_BACK_RIGHT | PAWIN_SPEAKER_SIDE_LEFT | PAWIN_SPEAKER_SIDE_RIGHT)

#define EXIT_ON_ERROR(hres) \
	if(FAILED(hres)) { \
		SPDLOG_ERROR("Failed to execute COM function at line {}: {:#x}", __LINE__, hres); \
		goto exit; \
	}
#define SAFE_RELEASE(punk) \
	if((punk) != NULL) { \
		(punk)->Release(); \
		(punk) = NULL; \
	}

HRESULT WasapiInstance::getDeviceByName(std::string name, IMMDevice** ppMMDevice) {
	HRESULT hr = S_OK;
	IMMDeviceEnumerator* pMMDeviceEnumerator = NULL;
	IMMDeviceCollection* pMMDeviceCollection = NULL;
	UINT count = 0;

	*ppMMDevice = nullptr;

	// activate a device enumerator
	hr = CoCreateInstance(
	    __uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**) &pMMDeviceEnumerator);
	EXIT_ON_ERROR(hr);

	hr = pMMDeviceEnumerator->EnumAudioEndpoints(eAll, DEVICE_STATEMASK_ALL, &pMMDeviceCollection);
	EXIT_ON_ERROR(hr);

	if(name == "default") {
		EDataFlow dataFlow;

		if(direction == D_Output) {
			dataFlow = eRender;
		} else {
			dataFlow = eCapture;
		}

		hr = pMMDeviceEnumerator->GetDefaultAudioEndpoint(dataFlow, eMultimedia, ppMMDevice);
		EXIT_ON_ERROR(hr);

	} else {
		hr = pMMDeviceCollection->GetCount(&count);
		EXIT_ON_ERROR(hr);

		for(UINT i = 0; i < count; i++) {
			IMMDevice* pEndpoint;
			IPropertyStore* pProps = NULL;
			PROPVARIANT varName;
			std::string deviceName;

			pMMDeviceCollection->Item(i, &pEndpoint);

			hr = pEndpoint->OpenPropertyStore(STGM_READ, &pProps);
			if(FAILED(hr)) {
				SPDLOG_ERROR("IMMDevice(OpenPropertyStore) failed: hr ={:#x}", hr);
				pEndpoint->Release();
				continue;
			}

			// Initialize container for property value.
			PropVariantInit(&varName);

			// Get the endpoint's friendly-name property.
			hr = pProps->GetValue(PKEY_Device_FriendlyName, &varName);
			if(FAILED(hr)) {
				SPDLOG_ERROR("IPropertyStore(GetValue) failed: hr = {:#x}", hr);
				pProps->Release();
				pEndpoint->Release();
				continue;
			}

			std::wstring_convert<std::codecvt_utf8_utf16<WCHAR>, WCHAR> convert;
			deviceName = convert.to_bytes(varName.pwszVal);

			PropVariantClear(&varName);
			pProps->Release();

			if(name == deviceName) {
				SPDLOG_INFO("Using device {}", deviceName);
				*ppMMDevice = pEndpoint;
				break;
			} else {
				pEndpoint->Release();
			}
		}
	}

	if(!*ppMMDevice) {
		SPDLOG_ERROR("Device {} not found\n", name);
		return E_NOTFOUND;
	}

exit:
	SAFE_RELEASE(pMMDeviceCollection);
	SAFE_RELEASE(pMMDeviceEnumerator);

	return hr;
}

std::vector<std::string> WasapiInstance::getDeviceList() {
	std::vector<std::string> result;
	HRESULT hr = S_OK;
	IMMDeviceEnumerator* pMMDeviceEnumerator = NULL;
	IMMDeviceCollection* pMMDeviceCollection = NULL;
	UINT count = 0;

	// activate a device enumerator
	hr = CoCreateInstance(
	    __uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**) &pMMDeviceEnumerator);
	EXIT_ON_ERROR(hr);

	hr = pMMDeviceEnumerator->EnumAudioEndpoints(eAll, DEVICE_STATEMASK_ALL, &pMMDeviceCollection);
	EXIT_ON_ERROR(hr);

	hr = pMMDeviceCollection->GetCount(&count);
	EXIT_ON_ERROR(hr);

	result.push_back("default");

	for(UINT i = 0; i < count; i++) {
		IMMDevice* pEndpoint;
		DWORD state;
		IPropertyStore* pProps = NULL;
		PROPVARIANT varName;
		std::string deviceName;

		pMMDeviceCollection->Item(i, &pEndpoint);

		hr = pEndpoint->GetState(&state);
		if(FAILED(hr) || state != DEVICE_STATE_ACTIVE) {
			pEndpoint->Release();
			continue;
		}

		hr = pEndpoint->OpenPropertyStore(STGM_READ, &pProps);
		if(FAILED(hr)) {
			SPDLOG_ERROR("IMMDevice(OpenPropertyStore) failed: hr = {:#x}", hr);
			pEndpoint->Release();
			continue;
		}

		// Initialize container for property value.
		PropVariantInit(&varName);

		// Get the endpoint's friendly-name property.
		hr = pProps->GetValue(PKEY_Device_FriendlyName, &varName);
		if(FAILED(hr)) {
			SPDLOG_ERROR("IPropertyStore(GetValue) failed: hr = {:#x}", hr);
			pProps->Release();
			pEndpoint->Release();
			continue;
		}

		std::wstring_convert<std::codecvt_utf8_utf16<WCHAR>, WCHAR> convert;
		deviceName = convert.to_bytes(varName.pwszVal);

		PropVariantClear(&varName);
		pProps->Release();

		result.push_back(deviceName);
		pEndpoint->Release();
	}

	if(count > 0)
		std::sort(result.begin() + 1, result.end());

exit:
	SAFE_RELEASE(pMMDeviceCollection);
	SAFE_RELEASE(pMMDeviceEnumerator);

	return result;
}

WasapiInstance::WasapiInstance(OscContainer* parent, Direction direction)
    : OscContainer(parent, "device"),
      oscDeviceName(this, "deviceName", "default"),
      oscBufferSize(this, "bufferSize", 0),
      oscActualBufferSize(this, "actualBufferSize", 0),
      oscDeviceSampleRate(this, "deviceSampleRate", 48000),
      oscClockDrift(this, "clockDrift", 0.0f),
      oscExclusiveMode(this, "exclusiveMode", true),
      deviceSampleRateMeasure(this, "realSampleRate") {
	this->direction = direction;

	// Only allow changes when stopped
	oscDeviceName.addCheckCallback([this](const std::string&) { return pDevice == nullptr; });
	oscBufferSize.addCheckCallback([this](int newValue) { return pDevice == nullptr && newValue >= 0; });
	oscDeviceSampleRate.addCheckCallback([this](int newValue) { return pDevice == nullptr && newValue > 0; });
	oscExclusiveMode.addCheckCallback([this](int) { return pDevice == nullptr; });

	oscClockDrift.setChangeCallback([this](float newValue) {
		for(auto& resamplingFilter : resamplingFilters) {
			resamplingFilter.setClockDrift(newValue);
		}
	});
	oscDeviceSampleRate.setChangeCallback([this, direction](float newValue) {
		for(auto& resamplingFilter : resamplingFilters) {
			if(direction == D_Output) {
				resamplingFilter.setTargetSamplingRate(newValue);
			} else {
				resamplingFilter.setSourceSamplingRate(newValue);
			}
		}
	});
}

WasapiInstance::~WasapiInstance() {
	WasapiInstance::stop();
}

const char* WasapiInstance::getName() {
	return "wasapi";
}

void WasapiInstance::stop() {
	if(pAudioClient) {
		pAudioClient->Stop();
	}

	SAFE_RELEASE(pRenderClient);
	SAFE_RELEASE(pCaptureClient);
	SAFE_RELEASE(pAudioClient);
	SAFE_RELEASE(pDevice);
}

int WasapiInstance::start(int index, size_t numChannel, int sampleRate, int jackBufferSize) {
	uint32_t hr = initializeWasapi(numChannel, sampleRate, jackBufferSize);
	if(FAILED(hr)) {
		SPDLOG_ERROR("Failed to initialize WASAPI: {:#x}", hr);
		return hr;
	}

	resampledBuffer.resize(numChannel);
	for(auto& buffer : resampledBuffer) {
		buffer.reserve(sampleRate);
	}
	resamplingFilters.resize(numChannel);
	for(size_t i = 0; i < numChannel; i++) {
		if(direction == D_Output) {
			resamplingFilters[i].reset(sampleRate);
			resamplingFilters[i].setTargetSamplingRate(oscDeviceSampleRate);
			resamplingFilters[i].setClockDrift(oscClockDrift);
		} else {
			resamplingFilters[i].reset(oscDeviceSampleRate);
			resamplingFilters[i].setTargetSamplingRate(sampleRate);
			resamplingFilters[i].setClockDrift(oscClockDrift);
		}
	}

	bufferLatencyMeasurePeriodSize = 60 * sampleRate / jackBufferSize;
	bufferLatencyNr = 0;
	bufferLatencyHistory.clear();
	bufferLatencyHistory.reserve(bufferLatencyMeasurePeriodSize);
	previousAverageLatency = 0;
	clockDriftPpm = 0;
	maxBufferSize = 0;

	hr = pAudioClient->Start();
	EXIT_ON_ERROR(hr);

	return 0;

exit:
	return hr;
}

uint32_t WasapiInstance::initializeWasapi(size_t numChannel, int jackSampleRate, int jackBufferSize) {
	HRESULT hr;
	WAVEFORMATEX* pFormat = nullptr;
	REFERENCE_TIME timePeriod = (uint64_t) jackBufferSize * REFTIMES_PER_SEC / jackSampleRate + 1;
	REFERENCE_TIME duration;
	REFERENCE_TIME bufferSize;

	hr = getDeviceByName(oscDeviceName, &pDevice);
	EXIT_ON_ERROR(hr);

	hr = pDevice->Activate(IID_IAudioClient, CLSCTX_ALL, NULL, (void**) &pAudioClient);
	EXIT_ON_ERROR(hr);

	hr = findAudioConfig(pAudioClient, numChannel, &pFormat);
	EXIT_ON_ERROR(hr);

	pAudioClient->GetDevicePeriod(nullptr, &duration);

	// A minimum buffer size of minimum period * 3
	bufferSize = std::max(std::max(duration * 3, timePeriod * 3),
	                      static_cast<REFERENCE_TIME>(static_cast<REFERENCE_TIME>(oscBufferSize) * REFTIMES_PER_SEC /
	                                                  oscDeviceSampleRate));
	SPDLOG_INFO("Asking buffer size: {}", bufferSize * oscDeviceSampleRate / REFTIMES_PER_SEC);

	if(oscExclusiveMode)
		SPDLOG_INFO("Using exclusive mode");

	SPDLOG_INFO("Using format:");
	printAudioConfig((WAVEFORMATEXTENSIBLE*) pFormat);

	hr = pAudioClient->Initialize(oscExclusiveMode ? AUDCLNT_SHAREMODE_EXCLUSIVE : AUDCLNT_SHAREMODE_SHARED,
	                              // useExclusiveMode ? AUDCLNT_STREAMFLAGS_EVENTCALLBACK : 0,
	                              0,
	                              bufferSize,
	                              oscExclusiveMode ? timePeriod : 0,
	                              pFormat,
	                              NULL);
	CoTaskMemFree(pFormat);
	EXIT_ON_ERROR(hr);

	//	if(useExclusiveMode) {
	//		audioEvent = CreateEvent(nullptr, true, false, "audio event");
	//		hr = pAudioClient->SetEventHandle(audioEvent);
	//		EXIT_ON_ERROR(hr);
	//	} else {
	audioEvent = nullptr;
	//	}

	if(direction == D_Output)
		hr = pAudioClient->GetService(IID_IAudioRenderClient, (void**) &pRenderClient);
	else
		hr = pAudioClient->GetService(IID_IAudioCaptureClient, (void**) &pCaptureClient);
	EXIT_ON_ERROR(hr);

	hr = pAudioClient->GetBufferSize(&wasapiBufferSize);
	EXIT_ON_ERROR(hr);

	oscActualBufferSize = wasapiBufferSize;
	SPDLOG_INFO("Using buffer size: {}", wasapiBufferSize);

	return 0;

exit:
	SAFE_RELEASE(pCaptureClient);
	SAFE_RELEASE(pRenderClient);
	SAFE_RELEASE(pAudioClient);
	SAFE_RELEASE(pDevice);

	return hr;
}

HRESULT WasapiInstance::findAudioConfig(IAudioClient* pAudioClient, size_t numChannel, WAVEFORMATEX** pFormat) {
	HRESULT hr;
	bool found = false;

	struct WaveFormat {
		Format format;
		uint32_t formatTag;
		size_t bitPerSample;
		size_t sampleSize;
	};

	static constexpr WaveFormat WAVE_FORMATS[] = {
	    {F_Float, WAVE_FORMAT_IEEE_FLOAT, 32, 4},
	    {F_Int32, WAVE_FORMAT_PCM, 32, 4},
	    {F_Int24, WAVE_FORMAT_PCM, 24, 3},
	    {F_Int16, WAVE_FORMAT_PCM, 16, 2},
	};

	if(oscExclusiveMode) {
		WAVEFORMATEXTENSIBLE* pWaveFormat;

		pWaveFormat = (WAVEFORMATEXTENSIBLE*) CoTaskMemAlloc(sizeof(WAVEFORMATEXTENSIBLE));

		for(const WaveFormat& format : WAVE_FORMATS) {
			pWaveFormat->Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
			pWaveFormat->Format.wBitsPerSample = format.bitPerSample;
			pWaveFormat->Format.nBlockAlign = pWaveFormat->Format.nChannels * format.sampleSize;
			pWaveFormat->Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);
			pWaveFormat->Samples.wValidBitsPerSample = format.bitPerSample;
			switch(numChannel) {
				case 1:
					pWaveFormat->dwChannelMask = PAWIN_SPEAKER_MONO;
					break;
				case 2:
					pWaveFormat->dwChannelMask = PAWIN_SPEAKER_STEREO;
					break;
				case 3:
					pWaveFormat->dwChannelMask = PAWIN_SPEAKER_STEREO | SPEAKER_LOW_FREQUENCY;
					break;
				case 4:
					pWaveFormat->dwChannelMask = PAWIN_SPEAKER_QUAD;
					break;
				case 5:
					pWaveFormat->dwChannelMask = PAWIN_SPEAKER_QUAD | SPEAKER_LOW_FREQUENCY;
					break;
				case 6:
					pWaveFormat->dwChannelMask = PAWIN_SPEAKER_5POINT1_SURROUND;
					break;
				case 7:
					pWaveFormat->dwChannelMask = PAWIN_SPEAKER_5POINT1_SURROUND | SPEAKER_BACK_CENTER;
					break;
				case 8:
					pWaveFormat->dwChannelMask = PAWIN_SPEAKER_7POINT1_SURROUND;
					break;

				default:
					pWaveFormat->dwChannelMask = 0;
			}

			INIT_WAVEFORMATEX_GUID(&pWaveFormat->SubFormat, format.formatTag);

			pWaveFormat->Format.nChannels = numChannel;
			pWaveFormat->Format.nSamplesPerSec = oscDeviceSampleRate;
			pWaveFormat->Format.nAvgBytesPerSec = pWaveFormat->Format.nBlockAlign * pWaveFormat->Format.nSamplesPerSec;

			hr = pAudioClient->IsFormatSupported(AUDCLNT_SHAREMODE_EXCLUSIVE, &pWaveFormat->Format, nullptr);
			if(hr == S_OK) {
				sampleFormat = format.format;
				*pFormat = &pWaveFormat->Format;
				found = true;
				break;
			}
		}

		if(!found)
			CoTaskMemFree(pWaveFormat);
	} else {
		WAVEFORMATEXTENSIBLE* pWaveFormat;

		hr = pAudioClient->GetMixFormat((WAVEFORMATEX**) &pWaveFormat);
		EXIT_ON_ERROR(hr);

		for(const WaveFormat& format : WAVE_FORMATS) {
			if((pWaveFormat->Format.wFormatTag == format.formatTag ||
			    (pWaveFormat->Format.wFormatTag == WAVE_FORMAT_EXTENSIBLE &&
			     EXTRACT_WAVEFORMATEX_ID(&pWaveFormat->SubFormat) == format.formatTag)) &&
			   pWaveFormat->Format.wBitsPerSample == format.bitPerSample &&
			   pWaveFormat->Format.nBlockAlign == pWaveFormat->Format.nChannels * format.sampleSize) {
				sampleFormat = format.format;
				*pFormat = &pWaveFormat->Format;
				found = true;
				break;
			}
		}

		oscDeviceSampleRate = pWaveFormat->Format.nSamplesPerSec;
	}

	if(!found) {
		SPDLOG_ERROR("Wasapi format unrecognized");
		hr = AUDCLNT_E_UNSUPPORTED_FORMAT;
		goto exit;
	}

	return 0;

exit:

	return hr;
}

void WasapiInstance::printAudioConfig(const WAVEFORMATEXTENSIBLE* pFormat) {
	if(!pFormat) {
		SPDLOG_ERROR("Invalid wave format NULL");
		return;
	}
	SPDLOG_INFO("- Format tag: {}", pFormat->Format.wFormatTag);
	SPDLOG_INFO("- Channels: {}", pFormat->Format.nChannels);
	SPDLOG_INFO("- Sample rate: {}", pFormat->Format.nSamplesPerSec);
	SPDLOG_INFO("- Avg bytes per sec: {}", pFormat->Format.nAvgBytesPerSec);
	SPDLOG_INFO("- Block align: {}", pFormat->Format.nBlockAlign);
	SPDLOG_INFO("- Bits per sample: {}", pFormat->Format.wBitsPerSample);
	SPDLOG_INFO("- cbSize: {}", pFormat->Format.cbSize);

	if(pFormat->Format.wFormatTag == WAVE_FORMAT_EXTENSIBLE) {
		SPDLOG_INFO("- Valid bits per sample: {}", pFormat->Samples.wValidBitsPerSample);
		SPDLOG_INFO("- Channel mask: {:#x}", pFormat->dwChannelMask);
		SPDLOG_INFO("- Format tag guid: {}", pFormat->SubFormat.Data1);
	}
}

int WasapiInstance::postProcessSamples(float** samples, size_t numChannel, uint32_t nframes) {
	if(audioEvent)
		ResetEvent(audioEvent);
	if(direction == D_Output)
		return postProcessSamplesRender(samples, numChannel, nframes);
	else
		return postProcessSamplesCapture(samples, numChannel, nframes);
}

void WasapiInstance::copySamplesRender(
    Format format, float** input, size_t numFrameToRead, void* output, size_t numChannel, uint32_t nframes) {
	if(resampledBuffer[0].size() <= resamplingFilters[0].getMaxRequiredOutputSize(nframes) * 3) {
		for(size_t i = 0; i < numChannel; i++) {
			float downSamplingRatio = resamplingFilters[i].getDownSamplingRatio();

			for(size_t j = 0; j < numFrameToRead; j++) {
				resamplingFilters[i].put(input[i][j]);
				resamplingFilters[i].get(resampledBuffer[i], downSamplingRatio);
			}
		}

		if(nframes > resampledBuffer[0].size()) {
			underflowOccured = true;
			underflowSize = resampledBuffer[0].size();
			return;
		}
	} else {
		overflowOccured = true;
		overflowSize = resampledBuffer[0].size();
		return;
	}

	if(format == F_Float) {
		float* data = (float*) output;

		for(size_t i = 0; i < numChannel; i++) {
			for(size_t j = 0; j < nframes; j++) {
				data[i + j * numChannel] = resampledBuffer[i][j];
			}
		}
	} else if(format == F_Int32) {
		int32_t* data = (int32_t*) output;

		for(size_t i = 0; i < numChannel; i++) {
			for(size_t j = 0; j < nframes; j++) {
				data[i + j * numChannel] = static_cast<int32_t>(resampledBuffer[i][j] * static_cast<float>(INT32_MAX));
			}
		}
	} else if(format == F_Int24) {
		uint8_t* data = (uint8_t*) output;

		for(size_t i = 0; i < numChannel; i++) {
			for(size_t j = 0; j < nframes; j++) {
				uint8_t* targetSampleValue = &data[(i + j * numChannel) * 3];
				uint32_t value = static_cast<uint32_t>(
				    static_cast<int32_t>(resampledBuffer[i][j] * static_cast<float>(INT32_MAX / 256)));
				targetSampleValue[0] = value;
				targetSampleValue[1] = value >> 8;
				targetSampleValue[2] = value >> 16;
			}
		}
	} else if(format == F_Int16) {
		int16_t* data = (int16_t*) output;

		for(size_t i = 0; i < numChannel; i++) {
			for(size_t j = 0; j < nframes; j++) {
				data[i + j * numChannel] = static_cast<int16_t>(resampledBuffer[i][j] * static_cast<float>(INT16_MAX));
			}
		}
	}

	for(size_t i = 0; i < numChannel; i++) {
		resampledBuffer[i].erase(resampledBuffer[i].begin(), resampledBuffer[i].begin() + nframes);
	}
}

int WasapiInstance::postProcessSamplesRender(float** samples, size_t numChannel, uint32_t nframes) {
	HRESULT hr;

	if(!pRenderClient)
		return 0;

	UINT32 validSize;
	size_t availableForWrite;
	size_t requiredBufferSize = resamplingFilters[0].getMinRequiredOutputSize(nframes) + resampledBuffer[0].size();
	// size_t requiredBufferSize = wasapiBufferSize;
	BYTE* buffer;

	hr = pAudioClient->GetCurrentPadding(&validSize);
	if(hr == AUDCLNT_E_DEVICE_INVALIDATED)
		return 0;
	EXIT_ON_ERROR(hr);

	// If validSize is 0, we can't assume we have an underrun. It could be just that we are fast enough to refill the
	// buffer just in time.

	availableForWrite = wasapiBufferSize - validSize;

	if(validSize != 0 && (maxBufferSize == 0 || maxBufferSize > validSize))
		maxBufferSize = validSize;

	if(availableForWrite >= requiredBufferSize) {
		hr = pRenderClient->GetBuffer(requiredBufferSize, &buffer);
		EXIT_ON_ERROR(hr);

		copySamplesRender(sampleFormat, samples, nframes, buffer, numChannel, requiredBufferSize);

		hr = pRenderClient->ReleaseBuffer(requiredBufferSize, 0);
		EXIT_ON_ERROR(hr);
		deviceSampleRateMeasure.notifySampleProcessed(requiredBufferSize);
	} else {
		overflowOccured = true;
		overflowSize = resampledBuffer[0].size();
	}

	if(underflowOccured || overflowOccured) {
		bufferLatencyHistory.clear();
	} else {
		bufferLatencyHistory.push_back(validSize + requiredBufferSize + resampledBuffer[0].size());

		if(bufferLatencyHistory.size() == bufferLatencyMeasurePeriodSize) {
			double averageLatency = 0;
			for(auto value : bufferLatencyHistory) {
				averageLatency += value;
			}
			averageLatency /= bufferLatencyHistory.size();
			bufferLatencyHistory.clear();

			if(previousAverageLatency != 0) {
				double sampleLatencyDiffOver1s = averageLatency - previousAverageLatency;
				double sampleMeasureDuration = bufferLatencyMeasurePeriodSize * nframes;

				// Ignore drift of more than 0.1%
				//				if(sampleLatencyDiffOver1s > (-sampleMeasureDuration / 1000) &&
				//				   sampleLatencyDiffOver1s < (sampleMeasureDuration / 1000)) {
				clockDriftPpm = 1000000.0 * sampleLatencyDiffOver1s / sampleMeasureDuration;
				//				}
			}

			previousAverageLatency = averageLatency;
		}
	}

	return 0;

exit:
	bufferLatencyHistory.clear();
	return 0;
}

void WasapiInstance::copySamplesCapture(
    Format format, void* input, size_t numFrameToRead, float** output, size_t numChannel, uint32_t nframes) {
	if(format == F_Float) {
		float* data = (float*) input;

		for(size_t i = 0; i < numChannel; i++) {
			float downSamplingRatio = resamplingFilters[i].getDownSamplingRatio();

			for(size_t j = 0; j < numFrameToRead; j++) {
				resamplingFilters[i].put(data[i + j * numChannel]);
				resamplingFilters[i].get(resampledBuffer[i], downSamplingRatio);
			}
		}
	} else if(format == F_Int32) {
		int32_t* data = (int32_t*) input;

		for(size_t i = 0; i < numChannel; i++) {
			float downSamplingRatio = resamplingFilters[i].getDownSamplingRatio();

			for(size_t j = 0; j < numFrameToRead; j++) {
				resamplingFilters[i].put(data[i + j * numChannel] / static_cast<float>(INT32_MAX));
				resamplingFilters[i].get(resampledBuffer[i], downSamplingRatio);
			}
		}
	} else if(format == F_Int24) {
		uint8_t* data = (uint8_t*) input;

		for(size_t i = 0; i < numChannel; i++) {
			float downSamplingRatio = resamplingFilters[i].getDownSamplingRatio();

			for(size_t j = 0; j < numFrameToRead; j++) {
				uint8_t* targetSampleValue = &data[(i + j * numChannel) * 3];
				int32_t value = (targetSampleValue[0] << 8 | targetSampleValue[1] << 16 | targetSampleValue[2] << 24);
				resamplingFilters[i].put(value / static_cast<float>(INT32_MAX));
				resamplingFilters[i].get(resampledBuffer[i], downSamplingRatio);
			}
		}
	} else if(format == F_Int16) {
		int16_t* data = (int16_t*) input;

		for(size_t i = 0; i < numChannel; i++) {
			float downSamplingRatio = resamplingFilters[i].getDownSamplingRatio();

			for(size_t j = 0; j < numFrameToRead; j++) {
				resamplingFilters[i].put(data[i + j * numChannel] / static_cast<float>(INT16_MAX));
				resamplingFilters[i].get(resampledBuffer[i], downSamplingRatio);
			}
		}
	}

	for(size_t i = 0; i < numChannel; i++) {
		if(resampledBuffer[i].size() >= nframes) {
			for(size_t j = 0; j < nframes; j++) {
				output[i][j] = resampledBuffer[i][j];
			}
			resampledBuffer[i].erase(resampledBuffer[i].begin(), resampledBuffer[i].begin() + nframes);
		} else {
			memset(output[i], 0, nframes * sizeof(output[i][0]));
			underflowOccured = true;
			underflowSize = resampledBuffer[i].size();
		}
	}
}

int WasapiInstance::postProcessSamplesCapture(float** samples, size_t numChannel, uint32_t nframes) {
	HRESULT hr;

	if(!pCaptureClient)
		return 0;

	UINT32 validSize;
	UINT32 numFrameToRead;
	DWORD flags;

	BYTE* buffer;

	hr = pAudioClient->GetCurrentPadding(&validSize);
	if(hr == AUDCLNT_E_DEVICE_INVALIDATED)
		return 0;
	EXIT_ON_ERROR(hr);

	hr = pCaptureClient->GetBuffer(&buffer, &numFrameToRead, &flags, nullptr, nullptr);
	EXIT_ON_ERROR(hr);

	if(flags & AUDCLNT_BUFFERFLAGS_DATA_DISCONTINUITY) {
		// Assume overflow
		overflowOccured = true;
		overflowSize = validSize;
	}

	if(validSize != 0 && (maxBufferSize == 0 || maxBufferSize > validSize))
		maxBufferSize = validSize;

	copySamplesCapture(sampleFormat, buffer, numFrameToRead, samples, numChannel, nframes);

	hr = pCaptureClient->ReleaseBuffer(numFrameToRead);
	EXIT_ON_ERROR(hr);
	deviceSampleRateMeasure.notifySampleProcessed(numFrameToRead);

	if(underflowOccured || overflowOccured) {
		bufferLatencyHistory.clear();
	} else {
		bufferLatencyHistory.push_back(resampledBuffer[0].size() + nframes);

		if(bufferLatencyHistory.size() == bufferLatencyMeasurePeriodSize) {
			double averageLatency = 0;
			for(auto value : bufferLatencyHistory) {
				averageLatency += value;
			}
			averageLatency /= bufferLatencyHistory.size();
			bufferLatencyHistory.clear();

			if(previousAverageLatency != 0) {
				double sampleLatencyDiffOver1s = averageLatency - previousAverageLatency;
				double sampleMeasureDuration = bufferLatencyMeasurePeriodSize * nframes;

				// Ignore drift of more than 0.1%
				//				if(sampleLatencyDiffOver1s > (-sampleMeasureDuration / 1000) &&
				//				   sampleLatencyDiffOver1s < (sampleMeasureDuration / 1000)) {
				clockDriftPpm = 1000000.0 * sampleLatencyDiffOver1s / sampleMeasureDuration;
				//				}
			}

			previousAverageLatency = averageLatency;
		}
	}

	return 0;

exit:
	bufferLatencyHistory.clear();
	return 0;
}

void WasapiInstance::onFastTimer() {
	if(!pAudioClient)
		return;

	if(overflowOccured) {
		SPDLOG_WARN("{}: Overflow: {}, {}", oscDeviceName.get(), bufferLatencyNr, overflowSize);
		overflowOccured = false;
	}
	if(underflowOccured) {
		SPDLOG_WARN("{}: underrun: {}, {}", oscDeviceName.get(), bufferLatencyNr, underflowSize);
		underflowOccured = false;
	}
	if(clockDriftPpm) {
		SPDLOG_INFO("{}: average latency: {}", oscDeviceName.get(), previousAverageLatency);
		SPDLOG_INFO("{}: drift: {}", oscDeviceName.get(), clockDriftPpm);
		clockDriftPpm = 0;
	}
}

void WasapiInstance::onSlowTimer() {
	deviceSampleRateMeasure.onTimeoutTimer();
}
