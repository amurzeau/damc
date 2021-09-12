#ifndef DELAYFILTER_H
#define DELAYFILTER_H

#include <stddef.h>
#include <vector>

class DelayFilter {
public:
	DelayFilter();

	void reset();
	void processSamples(float* output, const float* input, size_t count);
	float processOneSample(float input);

	void setParameters(unsigned int delay);
	void getParameters(unsigned int& delay) { delay = this->delay; }

private:
	std::vector<float> delayedSamples;
	unsigned int delay;
	size_t inputIndex;
	size_t outputIndex;
	size_t power2Size;
};

#endif
