#ifndef CONVOLVER_H

#include <array>
#include <vector>

class Convolver {
public:
	int setConvolver(float* samples, size_t bufferSize);
	void processSamples(float* out, float* in, size_t nframes);

private:
	std::vector<float> convolverSignal;
	std::array<float, 8192> history;
	size_t currentHistoryPos;
};

#endif
