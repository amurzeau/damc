#include "Convolver.h"
#include <algorithm>
#include <math.h>
#include <stdio.h>
#include <string.h>

int Convolver::setConvolver(float* samples, size_t bufferSize) {
	std::reverse_copy(samples, samples + bufferSize, std::back_inserter(convolverSignal));

	currentHistoryPos = 0;
	std::fill_n(history.begin(), history.size(), 0);

	if(bufferSize + convolverSignal.size() + 1 > history.size()) {
		printf(
		    "Pulse or buffer size too large: %d > %d\n", bufferSize + convolverSignal.size() + 1, (int) history.size());
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

	size_t historyAbsoluteBasePos = (historySize + currentHistoryPos - 1 - (nframes - 1) - (convolverSignalSize - 1));
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
		out[n] = sum / convolverSignalSize;
	}
}
