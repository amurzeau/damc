#pragma once

#include <complex>
#include <stddef.h>

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

class BiquadFilter {
public:
	void init(const double a_coefs[3], const double b_coefs[3]);
	void update(const double a_coefs[3], const double b_coefs[3]);
	double put(double input);

	static void computeFilter(bool enabled,
	                          FilterType filterType,
	                          float f0,
	                          float fs,
	                          float gain,
	                          float Q,
	                          double a_coefs[3],
	                          double b_coefs[3]);
	void computeFilter(bool enabled, FilterType filterType, float f0, float fs, float gain, float Q);
	std::complex<double> getResponse(double f0, double fs);

private:
	// Use offset of 0.5 to avoid denormals
	double s1 = 0.5;
	double s2 = 0.5;
	double b_coefs[3] = {};
	double a_coefs[2] = {};
};
