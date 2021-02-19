#ifndef EQFILTER_H
#define EQFILTER_H

#include "../json.h"
#include <complex>
#include <stddef.h>

class BiquadFilter {
public:
	void init(const double a_coefs[3], const double b_coefs[3]);
	void update(const double a_coefs[3], const double b_coefs[3]);
	double put(double input);

	std::complex<double> getResponse(double f0, double fs);

private:
	// Use offset of 0.5 to avoid denormals
	double s1 = 0.5;
	double s2 = 0.5;
	double b_coefs[3] = {};
	double a_coefs[2] = {};
};

class EqFilter {
public:
	enum class FilterType {
		None,
		LowPass,
		HighPass,
		BandPassConstantSkirt,
		BandPassConstantPeak,
		Notch,
		AllPass,
		Peak,
		LowShelf,
		HighShelf
	};

public:
	void init(size_t numChannel);
	void reset(double fs);
	void processSamples(float** output, const float** input, size_t count);

	void setParameters(bool enabled, FilterType filterType, double f0, double gain, double Q);
	void getParameters(bool& enabled, FilterType& filterType, double& f0, double& gain, double& Q) {
		enabled = this->enabled;
		filterType = this->filterType;
		f0 = this->f0;
		gain = this->gain;
		Q = this->Q;
	}

	void setParameters(const nlohmann::json& json);
	nlohmann::json getParameters();

	std::complex<double> getResponse(double f0);

private:
	bool enabled = false;
	FilterType filterType = FilterType::None;
	double f0 = 1000;
	double fs = 48000;
	double gain = 0;
	double Q = 0.5;

	std::vector<BiquadFilter> biquadFilters;

	void computeFilter();
};

#endif
