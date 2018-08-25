#ifndef FILTERINGCHAIN_H
#define FILTERINGCHAIN_H

#include "DelayFilter.h"
#include "DitheringFilter.h"
#include "EqFilter.h"
#include <array>
#include <stddef.h>
#include <tuple>

class FilterChain {
public:
	void init(double fs, size_t numChannel, size_t numEq);
	void processSamples(double* samples, size_t count, int channel);

	void setMute(bool mute) { this->mute = mute; }
	void setVolumeDb(double db);
	void setEqualizer(size_t index, EqFilter::FilterType type, double f0, double gain, double Q);
	void setDelay(double delay);
	void setDithering(double scale, int bitReduction);

	bool isMute() { return this->mute; }
	double getVolumeDb();
	bool getEqualizer(size_t index, EqFilter::FilterType& type, double& f0, double& gain, double& Q);
	double getDelay();
	void getDithering(double& scale, int& bitReduction);

	size_t getEqualizerNumber() { return eqFilters.size(); }

private:
	std::vector<DelayFilter> delayFilters;
	std::vector<std::pair<bool, std::vector<EqFilter>>> eqFilters;
	std::vector<double> volume;
	std::vector<DitheringFilter> ditheringFilters;
	bool mute;
};

#endif
