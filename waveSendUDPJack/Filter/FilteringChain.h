#ifndef FILTERINGCHAIN_H
#define FILTERINGCHAIN_H

#include "../OscAddress.h"
#include "../json.h"
#include "CompressorFilter.h"
#include "DelayFilter.h"
#include "DitheringFilter.h"
#include "EqFilter.h"
#include "ExpanderFilter.h"
#include "ReverbFilter.h"
#include <array>
#include <stddef.h>
#include <tuple>

class FilterChain : public OscContainer {
public:
	FilterChain(OscContainer* parent);

	void init(size_t numChannel);
	void reset(double fs);
	void processSamples(float* peakOutput, float** output, const float** input, size_t numChannel, size_t count);
	float processSideChannelSample(float input);

	void setParameters(const nlohmann::json& json);
	nlohmann::json getParameters();

private:
	std::vector<DelayFilter> delayFilters;
	OscContainerArray<ReverbFilter> reverbFilters;
	OscContainerArray<EqFilter> eqFilters;
	OscVariable<int32_t> delay;
	OscArray<float> volume;
	OscVariable<float> masterVolume;
	CompressorFilter compressorFilter;
	ExpanderFilter expanderFilter;
	OscVariable<bool> mute;
};

#endif
