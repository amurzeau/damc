#ifndef EQFILTER_H
#define EQFILTER_H

#include "OscAddress.h"
#include "../json.h"
#include "BiquadFilter.h"
#include <complex>
#include <stddef.h>

class EqFilter : public OscContainer {
public:
	EqFilter(OscContainer* parent, const std::string& name);

	void init(size_t numChannel);
	void reset(double fs);
	void processSamples(float** output, const float** input, size_t count);

	void setParameters(bool enabled, FilterType filterType, double f0, double gain, double Q);
	void getParameters(bool& enabled, FilterType& filterType, double& f0, double& gain, double& Q) {
		enabled = this->enabled;
		filterType = (FilterType) this->filterType.get();
		f0 = this->f0;
		gain = this->gain;
		Q = this->Q;
	}

	void setParameters(const nlohmann::json& json);
	nlohmann::json getParameters();

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
