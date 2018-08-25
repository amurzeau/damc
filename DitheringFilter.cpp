#include "DitheringFilter.h"

#include <math.h>
#include <string.h>
#include <time.h>

DitheringFilter::DitheringFilter() : dither1(0, 1), dither2(-1, 0) {
	randGenerator.seed(time(nullptr));
}

void DitheringFilter::init(double) {
	this->scale = 1;
	this->bitReduction = 0;
	bitRatio = 0;
	previousRandom = 0;
}

void DitheringFilter::processSamples(double* samples, size_t count) {
	if(bitReduction) {
		double quantizationError = previousQuantizationError;
		for(size_t i = 0; i < count; i++) {
			double r = (dither1(randGenerator) - 0.5) / bitRatio;
			double dither = previousRandom - r;
			previousRandom = r;
			if(scale == 0)
				dither = 0;
			double nonQuantizedOutput = samples[i] - dither /*+ scale * quantizationError*/;
			samples[i] = floor(nonQuantizedOutput * bitRatio) / bitRatio;
			quantizationError = samples[i] - nonQuantizedOutput;
		}
		previousQuantizationError = quantizationError;
	}
}

void DitheringFilter::setParameters(double scale, int bitReduction) {
	this->scale = scale;
	this->bitReduction = bitReduction;
	if(bitReduction > 0)
		bitRatio = pow(2, bitReduction - 1);
	else
		bitRatio = 0;
}
