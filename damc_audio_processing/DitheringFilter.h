#pragma once

#include <random>
#include <stddef.h>
#include <vector>

class DitheringFilter {
public:
	DitheringFilter();

	void reset(double fs);
	void processSamples(float* samples, size_t count);

	void setParameters(double scale, int bitReduction);
	void getParameters(double& scale, int& bitReduction) {
		scale = this->scale;
		bitReduction = this->bitReduction;
	}

private:
	double scale;
	int bitReduction;
	double bitRatio;
	double previousQuantizationError;
	double previousRandom;
	std::uniform_real_distribution<double> dither1;
	std::uniform_real_distribution<double> dither2;
	std::mt19937_64 randGenerator;
};
