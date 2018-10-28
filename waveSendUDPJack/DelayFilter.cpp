#include "DelayFilter.h"

#include <math.h>
#include <string.h>

void DelayFilter::init(double fs) {
	this->fs = fs;
	this->inputIndex = 0;
	this->outputIndex = 0;
	this->power2Size = 0;
	this->delayedSamples.resize(1 << power2Size);
}

void DelayFilter::processSamples(double* output, const double* input, size_t count) {
	double* delayBuffer = delayedSamples.data();

	for(size_t i = 0; i < count; i++) {
		delayBuffer[inputIndex] = input[i];
		output[i] = delayBuffer[outputIndex];
		inputIndex = (inputIndex + 1) & ((1 << power2Size) - 1);
		outputIndex = (outputIndex + 1) & ((1 << power2Size) - 1);
	}
}

void DelayFilter::setParameters(double delay) {
	int powerOfTwo;
	int targetDelay = size_t(delay);  // size_t(delay * fs);

	this->delay = delay;

	powerOfTwo = 0;
	while((1 << powerOfTwo) < (targetDelay + 1))
		powerOfTwo++;
	this->power2Size = powerOfTwo;
	this->delayedSamples.resize(1 << power2Size);
	outputIndex = (inputIndex + this->delayedSamples.size() - targetDelay) & ((1 << power2Size) - 1);
}
