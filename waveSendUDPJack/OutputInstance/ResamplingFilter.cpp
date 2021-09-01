#include "ResamplingFilter.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

bool ResamplingFilter::initialized;
std::array<double, 8192> ResamplingFilter::optimized_coefs;

void ResamplingFilter::reset(double fs) {
	if(!initialized) {
		initialized = true;

		for(size_t k = 0; k < oversamplingRatio; k++) {
			for(size_t i = 0; i < tapPerSample; i++) {
				optimized_coefs[i + (oversamplingRatio - 1 - k) * tapPerSample] =
				    coefs[k + i * oversamplingRatio] * oversamplingRatio;
			}
		}
	}

	baseSamplingRate = fs;
	targetSamplingRate = fs;
	currentPos = 0;
	previousDelay = oversamplingRatio - 1;
	history.fill(0);
}

void ResamplingFilter::put(double sample) {
	history[currentPos] = sample;
	currentPos = (currentPos - 1) % history.size();
}

int ResamplingFilter::get(std::vector<float>& out, double period) {
	double delay;
	int k = 0;

	for(delay = previousDelay; delay >= 0; delay -= period, k++) {
		out.push_back(getLinearInterpolatedPoint(delay));
	}

	previousDelay = delay + oversamplingRatio;
	return k;
}

int ResamplingFilter::processSamples(std::vector<float>& output, const float* input, size_t count) {
	output.clear();

	for(size_t i = 0; i < count; i++) {
		put(input[i]);
		get(output, downsamplingRatio);
	}

	return 0;
}

int ResamplingFilter::getNextOutputSize() {
	int iterations = 0;
	double delay;

	for(delay = previousDelay; delay >= 0; delay -= downsamplingRatio) {
		iterations++;
	}

	return iterations;
}

size_t ResamplingFilter::getMaxRequiredOutputSize(size_t count) {
	return ceil(count * oversamplingRatio / downsamplingRatio);
}

size_t ResamplingFilter::getMinRequiredOutputSize(size_t count) {
	return floor(count * oversamplingRatio / downsamplingRatio);
}

void ResamplingFilter::setClockDrift(float drift) {
	double newRatio = oversamplingRatio * baseSamplingRate / targetSamplingRate / (1.0 + (double) drift);
	if(newRatio >= 1)
		downsamplingRatio = newRatio;
}

float ResamplingFilter::getClockDrift() {
	return (oversamplingRatio * baseSamplingRate / targetSamplingRate / downsamplingRatio) - 1.0;
}

void ResamplingFilter::setSourceSamplingRate(float samplingRate) {
	baseSamplingRate = samplingRate;
	downsamplingRatio = oversamplingRatio * baseSamplingRate / targetSamplingRate;
}

float ResamplingFilter::getSourceSamplingRate() {
	return baseSamplingRate;
}

void ResamplingFilter::setTargetSamplingRate(float samplingRate) {
	targetSamplingRate = samplingRate;
	downsamplingRatio = oversamplingRatio * baseSamplingRate / targetSamplingRate;
}

float ResamplingFilter::getTargetSamplingRate() {
	return targetSamplingRate;
}

double ResamplingFilter::getLinearInterpolatedPoint(double delay) const {
	if(delay < 0 || delay > oversamplingRatio) {
		printf("Bad delay %f\n", delay);
	}
	int x = int(delay + 2) - 2;

	double rem = delay - x;

	if(rem == 0)
		return getOnePoint(x);

	double v1 = getOnePoint(x);
	double v2 = getOnePoint(x + 1);
	return v1 + rem * (v2 - v1);
}

double ResamplingFilter::getZeroOrderHoldInterpolatedPoint(double delay) const {
	return getOnePoint(int(delay));
}

double ResamplingFilter::getOnePoint(unsigned int delay) const {
	double sum = 0;
	unsigned int index = currentPos + (delay / oversamplingRatio);
	delay %= oversamplingRatio;

	for(unsigned int k = 0; k < tapPerSample; k++) {
		index = (index + 1) % history.size();
		sum += history[index] * optimized_coefs[k + delay * tapPerSample];
	}

	return sum;
}
