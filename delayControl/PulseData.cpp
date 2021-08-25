#include "PulseData.h"
#include "WavLoader.h"
#include <algorithm>

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <string>

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

void BiquadFilter::init(const double a_coefs[], const double b_coefs[]) {
	update(a_coefs, b_coefs);
}

void BiquadFilter::update(const double a_coefs[], const double b_coefs[]) {
	this->b_coefs[0] = b_coefs[0] / a_coefs[0];
	this->b_coefs[1] = b_coefs[1] / a_coefs[0];
	this->b_coefs[2] = b_coefs[2] / a_coefs[0];
	this->a_coefs[0] = a_coefs[1] / a_coefs[0];
	this->a_coefs[1] = a_coefs[2] / a_coefs[0];
}

double BiquadFilter::put(double input) {
	const double* const a = a_coefs;
	const double* const b = b_coefs;

	double y = b[0] * input + s1 - 0.5;
	s1 = s2 + b[1] * input + a[0] * y;
	s2 = b[2] * input + a[1] * y + 0.5;

	return y;
}

int PulseData::open(const std::string& pulseFilename, jack_nframes_t bufferSize, float sampleRate) {
	std::vector<int16_t> pulseRaw;
	std::vector<float> pulseResampled;

	unsigned wavSampleRate;

	if(WavLoader::load(pulseFilename, pulseRaw, wavSampleRate) != 0) {
		printf("Cannot load %s\n", pulseFilename.c_str());
		return -1;
	}

	std::transform(pulseRaw.begin(), pulseRaw.end(), std::back_inserter(pulseWave), [](int16_t sample) {
		return sample / 32768.0f;  // normalize
	});

	resamplingFilter.reset(sampleRate);
	resamplingFilter.setClockDrift(sampleRate / wavSampleRate);
	resamplingFilter.processSamples(pulseResampled, pulseWave.data(), pulseWave.size());
	pulseWave.swap(pulseResampled);

	printf("Load pulse %s, %d samples at %d hz, resampled to %.3f hz with %d samples\n",
	       pulseFilename.c_str(),
	       (int) pulseRaw.size(),
	       wavSampleRate,
	       sampleRate,
	       (int) pulseWave.size());

	if(bufferSize + pulseWave.size() + 1 > history.size()) {
		printf("Pulse or buffer size too large: %d > %d\n",
		       (int) (bufferSize + pulseWave.size() + 1),
		       (int) history.size());
		return -1;
	}

	this->sampleRate = sampleRate;
	currentHistoryPos = 0;
	std::fill_n(history.begin(), history.size(), 0);
	currentFilteredHistoryPos = 0;
	previousPeak = 0;
	currentTime = 0;
	memset(filteredHistory, 0, sizeof(filteredHistory));
	std::fill_n(peakHistory.begin(), peakHistory.size(), std::make_pair(-1.0f, 0.0f));
	currentPeakHistoryPos = 0;
	insidePeak = false;
	snapshotedThreshold = 0;
	resamplingFilter.reset(sampleRate);

	uv_mutex_init(&detectedPulsesMutex);

	return 0;
}

jack_default_audio_sample_t PulseData::doCFAR(int delay) {
	static const int gap_samples = 0.0005 * sampleRate;
	static const int cell_samples = 0.015 * sampleRate;
	static const int filteredHistorySize = sizeof(filteredHistory) / sizeof(filteredHistory[0]);

	float noise_sum = 0;
	for(int i = gap_samples; i < gap_samples + cell_samples; i++) {
		noise_sum += filteredHistory[(filteredHistorySize + currentFilteredHistoryPos - 1 - i - delay) &
		                             (filteredHistorySize - 1)];
	}

	return noise_sum / cell_samples;
}

static std::pair<float, float> findPeakPosition(float& previousSample,
                                                float& previousDerivative,
                                                const std::vector<float>& samples) {
	float currentDerivative;
	std::pair<float, float> pos = std::make_pair(-1.0f, 0);

	for(size_t i = 0; i < samples.size(); i++) {
		currentDerivative = samples[i] - previousSample;

		if((samples[i] < 0 && previousDerivative < 0 && currentDerivative >= 0) ||
		   (samples[i] > 0 && previousDerivative >= 0 && currentDerivative < 0)) {
			// we crossed 0
			pos = std::make_pair(i / (float) samples.size(), fabsf(previousSample));
		}
		previousSample = samples[i];
		previousDerivative = currentDerivative;
	}

	return pos;
}

void PulseData::doAGC(jack_default_audio_sample_t in,
                      jack_default_audio_sample_t& out1,
                      jack_default_audio_sample_t& out2,
                      jack_default_audio_sample_t& out3) {
	static const int filteredHistorySize = sizeof(filteredHistory) / sizeof(filteredHistory[0]);
	static std::vector<float> resampledBuffer;
	// static float previousMaxElt = 0;

	float maxElt;

	resamplingFilter.put(in);
	resampledBuffer.clear();
	resamplingFilter.get(resampledBuffer, sampleRate * 128 / 384000);

	// Reconstruct enveloppe by :
	//  upsampling input signal to get peaks as close as possible to real sine peaks
	//  find real peaks position in sample count in float (for sub-sample precision)
	//  interpolate the current 48khz-sample from 384khz-peaks
	peakHistory[currentPeakHistoryPos] = findPeakPosition(previousSample, previousDerivative, resampledBuffer);
	currentPeakHistoryPos = (currentPeakHistoryPos + 1) % peakHistory.size();

	std::pair<float, float> peak1, peak2;
	peak1 = peakHistory[(currentPeakHistoryPos - 3) % peakHistory.size()];
	if(peak1.first < 0) {
		peak1 = peakHistory[(currentPeakHistoryPos - 4) % peakHistory.size()];
	} else {
		peak1.first += 1.0f;
	}
	peak2 = peakHistory[(currentPeakHistoryPos - 2) % peakHistory.size()];
	if(peak2.first < 0) {
		peak2 = peakHistory[(currentPeakHistoryPos - 1) % peakHistory.size()];
		peak2.first += 3.0f;
	} else {
		peak2.first += 2.0f;
	}

	if(peak1.first >= 0 && peak2.first >= 0) {
		float targetSamplePos = (2 - peak1.first) / (peak2.first - peak1.first);
		maxElt = peak1.second * (1 - targetSamplePos) + peak2.second * targetSamplePos;
	} else {
		maxElt = 0;
	}

	filteredHistory[currentFilteredHistoryPos] = maxElt;
	currentFilteredHistoryPos = (currentFilteredHistoryPos + 1) & (filteredHistorySize - 1);

	static const int pulseDetectDelay = 0.0012 * sampleRate;
	const float threshold = doCFAR(pulseDetectDelay) * 4;
	maxElt = filteredHistory[(currentFilteredHistoryPos - 1 - pulseDetectDelay) & (filteredHistorySize - 1)];
	if(maxElt > threshold && currentTime > previousPeak + uint64_t(0.030 * sampleRate)) {
		uint64_t detectTime = currentTime;
		float maxValue = 0;

		for(int i = 0; i < pulseDetectDelay; i++) {
			float value =
			    filteredHistory[(filteredHistorySize + currentFilteredHistoryPos - 1 - i) & (filteredHistorySize - 1)];
			if(value > maxValue) {
				maxValue = value;
				detectTime = currentTime + pulseDetectDelay - i;
			}
		}

		uv_mutex_lock(&detectedPulsesMutex);
		detectedPulses.push_back(detectTime);
		uv_mutex_unlock(&detectedPulsesMutex);
		previousPeak = detectTime;

		insidePeak = true;
		snapshotedThreshold = threshold;
	} else if(insidePeak && maxElt <= snapshotedThreshold) {
		insidePeak = false;
	}
	out1 = filteredHistory[(currentFilteredHistoryPos - 1) & (filteredHistorySize - 1)] * 100 *
	       ((currentTime == previousPeak) ? -1 : 1);  // * (0.001 / meanAgc);
	out2 = threshold * 100;                           // threshold * 100;
	out3 = resampledBuffer[0] * 100;

	// previousMaxElt = maxElt;
}

void PulseData::doCrossCorrelation(jack_default_audio_sample_t* in,
                                   jack_default_audio_sample_t* out1,
                                   jack_default_audio_sample_t* out2,
                                   jack_default_audio_sample_t* out3,
                                   jack_nframes_t nframes) {
	size_t pulseSize = this->pulseWave.size();
	static const int historySize = this->history.size();
	jack_default_audio_sample_t* pulseWave = &this->pulseWave[0];
	jack_default_audio_sample_t* history = &this->history[0];
	size_t iframes = nframes;

	//	for(int n = -(pulseSize - 1); n < iframes - (pulseSize - 1); n++) {
	//		float sum = 0;
	//		for(int m = 0; m < pulseSize; m++) {
	//			if(m + n >= 0 && m + n < iframes)
	//				sum += pulseWave[m] * in[m + n];
	//			else if(historySize + m + n >= 0 && historySize + m + n < historySize)
	//				sum += pulseWave[m] * history[historySize + m + n];
	//			else
	//				printf("Bad index: %d\n", m + n);
	//		}
	//		doAGC(sum / pulseSize, out1[n + (pulseSize - 1)], out2[n + (pulseSize - 1)], out3[n + (pulseSize - 1)]);
	//		currentTime++;
	//	}

	//	for(int i = 0; i < historySize; i++) {
	//		if(i + iframes - historySize >= 0)
	//			history[i] = in[i + iframes - historySize];
	//		else if(i + iframes >= 0 && i + iframes < historySize)
	//			history[i] = history[i + iframes];
	//		else
	//			printf("Bad index: %d\n", i);
	//	}

	if((currentHistoryPos + iframes - 1) % historySize > currentHistoryPos) {
		// No wrap around, just copy
		std::copy_n(in, iframes, history + currentHistoryPos);
		currentHistoryPos = (currentHistoryPos + iframes) % historySize;
	} else {
		// Manual sample by sample copy
		for(size_t i = 0; i < iframes; i++) {
			history[currentHistoryPos] = in[i];
			currentHistoryPos = (currentHistoryPos + 1) % historySize;
		}
	}

	size_t historyAbsoluteBasePos = (historySize + currentHistoryPos - 1 - (iframes - 1) - (pulseSize - 1));
	for(size_t n = 0; n < iframes; n++) {
		float sum = 0;
		for(size_t m = 0; m < pulseSize;) {
			size_t historyBasePos = static_cast<size_t>(historyAbsoluteBasePos + n + m) % historySize;
			size_t chunkSize = pulseSize - m;

			if(chunkSize > historySize - historyBasePos)
				chunkSize = historySize - historyBasePos;

			for(size_t i = 0; i < chunkSize; i++, m++) {
				sum += pulseWave[m] * history[historyBasePos + i];
			}
		}
		doAGC(sum / pulseSize, out1[n], out2[n], out3[n]);
		currentTime++;
	}
}

std::vector<uint64_t> PulseData::retrieveDetectedPulses() {
	std::vector<uint64_t> result;
	uv_mutex_lock(&detectedPulsesMutex);
	result = detectedPulses;
	detectedPulses.clear();
	uv_mutex_unlock(&detectedPulsesMutex);
	return result;
}
