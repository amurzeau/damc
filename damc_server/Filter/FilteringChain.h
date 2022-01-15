#pragma once

#include "CompressorFilter.h"
#include "DelayFilter.h"
#include "DitheringFilter.h"
#include "EqFilter.h"
#include "ExpanderFilter.h"
#include "PeakMeter.h"
#include "ReverbFilter.h"
#include <Osc/OscArray.h>
#include <Osc/OscContainer.h>
#include <Osc/OscContainerArray.h>
#include <Osc/OscVariable.h>
#include <stddef.h>

class FilterChain : public OscContainer {
public:
	FilterChain(OscContainer* parent,
	            OscReadOnlyVariable<int32_t>* oscNumChannel,
	            OscReadOnlyVariable<int32_t>* oscSampleRate);

	void reset(double fs);
	void processSamples(float** output, const float** input, size_t numChannel, size_t count);
	float processSideChannelSample(float input);

	void onFastTimer();

protected:
	void updateNumChannels(size_t numChannel);

private:
	std::vector<DelayFilter> delayFilters;
	OscContainerArray<ReverbFilter> reverbFilters;
	OscContainerArray<EqFilter> eqFilters;
	CompressorFilter compressorFilter;
	ExpanderFilter expanderFilter;
	PeakMeter peakMeter;

	OscVariable<int32_t> delay;
	OscArray<float> volume;
	OscVariable<float> masterVolume;
	OscVariable<bool> mute;
	OscVariable<bool> reverseAudioSignal;
};
