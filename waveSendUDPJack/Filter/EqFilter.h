#ifndef EQFILTER_H
#define EQFILTER_H

#include "BiquadFilter.h"
#include <Osc/OscContainer.h>
#include <Osc/OscVariable.h>
#include <complex>
#include <stddef.h>

class EqFilter : public OscContainer {
public:
	EqFilter(OscContainer* parent, const std::string& name);

	void init(size_t numChannel);
	void reset(double fs);
	void processSamples(float** output, const float** input, size_t count);

	void setParameters(bool enabled, FilterType filterType, double f0, double gain, double Q);
	void getParameters(bool& enabled, FilterType& filterType, double& f0, double& gain, double& Q);

	std::complex<double> getResponse(double f0);

private:
	OscVariable<bool> enabled;
	OscVariable<int32_t> filterType;
	OscVariable<float> f0;
	float fs = 48000;
	OscVariable<float> gain;
	OscVariable<float> Q;

	std::vector<BiquadFilter> biquadFilters;

	void computeFilter();
};

#endif
