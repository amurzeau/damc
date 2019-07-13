#ifndef FILTERINGCHAIN_H
#define FILTERINGCHAIN_H

#include "../json.h"
#include "CompressorFilter.h"
#include "DelayFilter.h"
#include "DitheringFilter.h"
#include "EqFilter.h"
#include "ReverbFilter.h"
#include <array>
#include <stddef.h>
#include <tuple>

class FilterChain {
public:
	FilterChain();

	void init(size_t numChannel);
	void reset(double fs);
	void processSamples(float* peakOutput, float** output, const float** input, size_t numChannel, size_t count);
	float processSideChannelSample(float input);

	void setMute(bool mute) { this->mute = mute; }
	void setEqualizer(size_t index, EqFilter::FilterType type, double f0, double gain, double Q);

	bool isMute() { return this->mute; }
	bool getEqualizer(size_t index, EqFilter::FilterType& type, double& f0, double& gain, double& Q);

	size_t getEqualizerNumber() { return eqFilters.size(); }

	void setParameters(const nlohmann::json& json);
	nlohmann::json getParameters();

private:
	std::vector<DelayFilter> delayFilters;
	std::vector<ReverbFilter> reverbFilters;
	std::vector<std::pair<bool, std::vector<EqFilter>>> eqFilters;
	std::vector<float> volume;
	float masterVolume;
	CompressorFilter compressorFilter;
	bool mute;
	bool enabled;
};

#endif
