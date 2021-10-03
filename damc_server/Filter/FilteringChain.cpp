#include "FilteringChain.h"
#include <math.h>
#include <string.h>

float LogScaleFromOsc(float value) {
	return powf(10, value / 20.0f);
}

float LogScaleToOsc(float value) {
	return 20.0 * log10(value);
}

FilterChain::FilterChain(OscContainer* parent,
                         OscReadOnlyVariable<int32_t>* oscNumChannel,
                         OscReadOnlyVariable<int32_t>* oscSampleRate)
    : OscContainer(parent, "filterChain"),
      reverbFilters(this, "reverbFilter"),
      eqFilters(this, "eqFilters"),
      compressorFilter(this),
      expanderFilter(this),
      peakMeter(parent, oscNumChannel, oscSampleRate),
      delay(this, "delay", 0),
      volume(this, "balance", 1.0f),
      masterVolume(this, "volume", 1.0f),
      mute(this, "mute", false) {
	reverbFilters.setFactory(
	    [](OscContainer* parent, int name) { return new ReverbFilter(parent, std::to_string(name)); });
	eqFilters.setFactory([](OscContainer* parent, int name) { return new EqFilter(parent, std::to_string(name)); });

	delay.setChangeCallback([this](int32_t newValue) {
		for(DelayFilter& filter : delayFilters) {
			filter.setParameters(newValue);
		}
	});
	volume.setOscConverters(&LogScaleToOsc, &LogScaleFromOsc);
	masterVolume.setOscConverters(&LogScaleToOsc, &LogScaleFromOsc);

	oscNumChannel->setChangeCallback([this](int32_t newValue) {
		if(newValue > 0)
			updateNumChannels(newValue);
	});
}

void FilterChain::updateNumChannels(size_t numChannel) {
	delayFilters.resize(numChannel + 1);  // +1 for side channel
	reverbFilters.resize(numChannel);
	volume.resize(numChannel);

	eqFilters.resize(6);

	for(auto& filter : eqFilters) {
		filter.second->init(numChannel);
	}

	compressorFilter.init(numChannel);
	expanderFilter.init(numChannel);
}

void FilterChain::reset(double fs) {
	for(DelayFilter& delayFilter : delayFilters) {
		delayFilter.reset();
	}
	for(auto& reverbFilter : reverbFilters) {
		reverbFilter.second->reset();
	}

	for(auto& filter : eqFilters) {
		filter.second->reset(fs);
	}

	compressorFilter.reset(fs);
	expanderFilter.reset(fs);
}

void FilterChain::processSamples(float** output, const float** input, size_t numChannel, size_t count) {
	float masterVolume = this->masterVolume.get();
	float peaks[numChannel];

	for(uint32_t channel = 0; channel < numChannel; channel++) {
		delayFilters[channel].processSamples(output[channel], input[channel], count);
	}

	for(auto& filter : eqFilters) {
		filter.second->processSamples(output, const_cast<const float**>(output), count);
	}

	expanderFilter.processSamples(output, const_cast<const float**>(output), count);
	compressorFilter.processSamples(output, const_cast<const float**>(output), count);

	for(uint32_t channel = 0; channel < numChannel; channel++) {
		reverbFilters.at(channel).processSamples(output[channel], output[channel], count);
	}

	for(uint32_t channel = 0; channel < numChannel; channel++) {
		float volume = this->volume.at(channel).get() * masterVolume;
		float peak = 0;
		for(size_t i = 0; i < count; i++) {
			output[channel][i] *= volume;
			peak = fmaxf(peak, fabsf(output[channel][i]));
		}
		peaks[channel] = peak;
	}

	peakMeter.processSamples(peaks, numChannel, count);

	if(mute) {
		for(uint32_t channel = 0; channel < numChannel; channel++) {
			std::fill_n(output[channel], count, 0);
		}
	}
}

float FilterChain::processSideChannelSample(float input) {
	return delayFilters.back().processOneSample(input);
}

void FilterChain::onFastTimer() {
	peakMeter.onFastTimer();
}
