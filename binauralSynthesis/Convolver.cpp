#include "Convolver.h"
#include <algorithm>
#include <complex>
#include <math.h>
#include <stdio.h>
#include <string.h>

int Convolver::setConvolver(float* samples, size_t bufferSize) {
	convolverSignal.clear();
	std::reverse_copy(samples, samples + bufferSize, std::back_inserter(convolverSignal));

	currentHistoryPos = 0;
	std::fill_n(history.begin(), history.size(), 0);

	if(bufferSize + convolverSignal.size() + 1 > history.size()) {
		printf("Pulse or buffer size too large: %d > %d\n",
		       (int) (bufferSize + convolverSignal.size() + 1),
		       (int) history.size());
		return -1;
	}

	return 0;
}

void Convolver::processSamples(float* out, float* in, size_t nframes) {
	const size_t convolverSignalSize = this->convolverSignal.size();
	static const size_t historySize = this->history.size();
	const float* convolverSignal = &this->convolverSignal[0];
	float* history = &this->history[0];

	// Copy input to history
	size_t nextCopySize = std::min(historySize - currentHistoryPos, nframes);
	std::copy_n(in, nextCopySize, history + currentHistoryPos);
	currentHistoryPos = (currentHistoryPos + nextCopySize) % historySize;
	nextCopySize = nframes - nextCopySize;
	std::copy_n(in, nextCopySize, history + currentHistoryPos);
	currentHistoryPos = (currentHistoryPos + nextCopySize) % historySize;

	// Base pos = first new sample - (convolverSignalSize - 1)
	// We are not doing "- convolverSignalSize" because the last convoluted sample is to be convoluted with the new
	// sample
	size_t historyAbsoluteBasePos = (historySize + currentHistoryPos - nframes - (convolverSignalSize - 1));
	for(size_t n = 0; n < nframes; n++) {
		float sum = 0;
		for(size_t m = 0; m < convolverSignalSize;) {
			size_t historyBasePos = static_cast<size_t>(historyAbsoluteBasePos + n + m) % historySize;
			size_t chunkSize = convolverSignalSize - m;

			if(chunkSize > historySize - historyBasePos)
				chunkSize = historySize - historyBasePos;

			for(size_t i = 0; i < chunkSize; i++, m++) {
				sum += convolverSignal[m] * history[historyBasePos + i];
			}
		}
		out[n] = sum;
	}
}

float Convolver::getGain(float freq, float sampleRate) const {
	const size_t convolverSignalSize = this->convolverSignal.size();
	float sum = 0;

	for(size_t i = 0; i < convolverSignalSize; i++) {
		// convolver signal is reversed
		sum += std::abs(this->convolverSignal[i] *
		                std::exp(std::complex<float>(0, -(2 * M_PI * freq / sampleRate) * (convolverSignalSize - i))));
	}

	return sum;
}

void Convolver::normalize(float multiplier) {
	const size_t convolverSignalSize = this->convolverSignal.size();
	for(size_t i = 0; i < convolverSignalSize; i++) {
		this->convolverSignal[i] *= multiplier;
	}
}

void Convolver::normalize(float* normalizer) {
	const ssize_t convolverSignalSize = this->convolverSignal.size();
	const ssize_t normalizerSignalSize = convolverSignalSize;
	std::vector<float> normalizedResult;

	normalizedResult.resize(convolverSignalSize);

	for(ssize_t x = 0; x < (ssize_t) normalizedResult.size(); x++) {
		float sum = 0;
		for(ssize_t i = std::max(ssize_t(0), x - (convolverSignalSize - 1)); i < normalizerSignalSize && i < x; i++) {
			sum += this->convolverSignal[convolverSignalSize - 1 + i - x] * normalizer[i];
		}
		normalizedResult[x] = sum;
	}
	setConvolver(normalizedResult.data(), normalizedResult.size());
}

void Convolver::extendLowFrequencies() {}
