#include "EqFilter.h"

#include <algorithm>
#include <cmath>
#include <string.h>

EqFilter::EqFilter(OscContainer* parent, const std::string& name)
    : OscContainer(parent, name),
      enabled(this, "enable", false),
      filterType(this, "type", (int32_t) FilterType::None),
      f0(this, "f0", 1000),
      gain(this, "gain", 0),
      Q(this, "Q", 0.5) {
	auto onChangeCallback = [this](auto) { computeFilter(); };
	enabled.setChangeCallback(onChangeCallback);
	filterType.setChangeCallback(onChangeCallback);
	f0.setChangeCallback(onChangeCallback);
	gain.setChangeCallback(onChangeCallback);
	Q.setChangeCallback(onChangeCallback);
}

void EqFilter::init(size_t numChannel) {
	biquadFilters.resize(numChannel);
}

void EqFilter::reset(double fs) {
	this->fs = fs;
	computeFilter();
}

void EqFilter::processSamples(float** output, const float** input, size_t count) {
	if(enabled) {
		for(size_t channel = 0; channel < biquadFilters.size(); channel++) {
			BiquadFilter& biquadFilter = biquadFilters[channel];
			float* outputChannel = output[channel];
			const float* inputChannel = input[channel];

			for(size_t i = 0; i < count; i++) {
				outputChannel[i] = biquadFilter.put(inputChannel[i]);
			}
		}
	} else if(output != input) {
		for(size_t channel = 0; channel < biquadFilters.size(); channel++) {
			std::copy_n(input[channel], count, output[channel]);
		}
	}
}

std::complex<double> EqFilter::getResponse(double f0) {
	return biquadFilters.front().getResponse(f0, fs);
}

void EqFilter::computeFilter() {
	double a_coefs[3];
	double b_coefs[3];

	BiquadFilter::computeFilter(enabled, (FilterType) filterType.get(), f0, fs, gain, Q, a_coefs, b_coefs);

	for(BiquadFilter& biquadFilter : biquadFilters)
		biquadFilter.update(a_coefs, b_coefs);
}

void EqFilter::setParameters(bool enabled, FilterType filterType, double f0, double gain, double Q) {
	this->enabled = enabled;
	this->filterType = (int32_t) filterType;
	this->f0 = f0;
	this->gain = gain;
	this->Q = Q;
	computeFilter();
}

void EqFilter::getParameters(bool& enabled, FilterType& filterType, double& f0, double& gain, double& Q) {
	enabled = this->enabled;
	filterType = (FilterType) this->filterType.get();
	f0 = this->f0;
	gain = this->gain;
	Q = this->Q;
}
