#ifndef DELAYFILTER_H
#define DELAYFILTER_H

#include <stddef.h>
#include <vector>

class DelayFilter {
public:
	void init(double fs);
	void processSamples(double* output, const double* input, size_t count);

	void setParameters(double delay);
	void getParameters(double& delay) { delay = this->delay; }

private:
	std::vector<double> delayedSamples;
	double delay;
	double fs;
	size_t inputIndex;
	size_t outputIndex;
	size_t power2Size;
};

#endif
