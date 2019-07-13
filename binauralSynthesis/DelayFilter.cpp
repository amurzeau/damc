#include "DelayFilter.h"

#include <math.h>
#include <string.h>

void DelayFilter::init() {
	this->inputIndex = 0;
	this->outputIndex = 0;
	this->power2Size = 0;
	this->delayedSamples.resize(1 << power2Size, 0);
}

void DelayFilter::processSamples(float* output, const float* input, size_t count) {
	for(size_t i = 0; i < count; i++) {
		output[i] = processOneSample(input[i]);
	}
}

float DelayFilter::processOneSample(float input) {
	delayedSamples[inputIndex] = input;
	float output = delayedSamples[outputIndex];
	inputIndex = (inputIndex + 1) & ((1 << power2Size) - 1);
	outputIndex = (outputIndex + 1) & ((1 << power2Size) - 1);

	return output;
}

void DelayFilter::setParameters(unsigned int delay) {
	int powerOfTwo;
	int targetDelay = delay;

	this->delay = delay;

	powerOfTwo = 0;
	while((1 << powerOfTwo) < (targetDelay + 1))
		powerOfTwo++;
	this->power2Size = powerOfTwo;
	this->delayedSamples.resize(1 << power2Size);
	outputIndex = (inputIndex + this->delayedSamples.size() - targetDelay) & ((1 << power2Size) - 1);
}
