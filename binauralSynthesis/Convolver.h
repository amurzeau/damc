#ifndef CONVOLVER_H

#include <array>
#include <vector>

class Convolver {
public:
	int setConvolver(float* samples, size_t bufferSize);
	void processSamples(float* out, float* in, size_t nframes);
	float getGain(float freq, float sampleRate) const;
	void normalize(float multiplier);
	const std::vector<float>& getData() const { return convolverSignal; }
	void normalize(float* normalizer);
	void extendLowFrequencies();

private:
	std::vector<float> convolverSignal;
	std::array<float, 8192> history;
	size_t currentHistoryPos;
};

#endif
