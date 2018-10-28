#ifndef EQFILTER_H
#define EQFILTER_H

#include "json.h"
#include <complex>
#include <stddef.h>

class BiquadFilter {
public:
	void init(const double a_coefs[3], const double b_coefs[3]);
	void update(const double a_coefs[3], const double b_coefs[3]);
	double put(double input);

	std::complex<double> getResponse(double f0, double fs);

private:
	double s1 = 0;
	double s2 = 0;
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
	void init(FilterType filterType, double f0, double fs, double gain, double Q);
	void processSamples(double* output, const double* input, size_t count);

	void setParameters(FilterType filterType, double f0, double gain, double Q);
	void getParameters(FilterType& filterType, double& f0, double& gain, double& Q) {
		filterType = this->filterType;
		f0 = this->f0;
		gain = this->gain;
		Q = this->Q;
	}

	void setParameters(const nlohmann::json& json);
	nlohmann::json getParameters();

	std::complex<double> getResponse(double f0);

private:
	FilterType filterType = FilterType::None;
	double f0 = 1000;
	double fs = 48000;
	double gain = 0;
	double Q = 0.5;

	BiquadFilter biquadFilter;

	void computeFilter();
};

#endif
