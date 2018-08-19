#include "FilteringChain.h"
#include <math.h>
#include <string.h>

void FilterChain::init(double fs, size_t numChannel, size_t numEq) {
	delayFilters.resize(numChannel);
	eqFilters.resize(numEq);

	for(DelayFilter& delayFilter : delayFilters) {
		delayFilter.init(fs);
	}
	for(std::pair<bool, std::vector<EqFilter>>& filter : eqFilters) {
		filter.first = false;
		filter.second.resize(numChannel);
		for(EqFilter& eqFilter : filter.second) {
			eqFilter.init(EqFilter::FilterType::None, 1000, fs, 0, 0);
		}
	}

	volume.resize(numChannel, 1);
}

void FilterChain::processSamples(double* samples, size_t count, int channel) {
	delayFilters[channel].processSamples(samples, samples, count);

	for(std::pair<bool, std::vector<EqFilter>>& filter : eqFilters) {
		if(filter.first) {
			filter.second[channel].processSamples(samples, samples, count);
		}
	}

	if(!mute) {
		double volume = this->volume[channel];
		for(size_t i = 0; i < count; i++) {
			samples[i] *= volume;
		}
	} else {
		for(size_t i = 0; i < count; i++) {
			samples[i] = 0;
		}
	}
}

void FilterChain::setVolumeDb(double db) {
	for(double& volumeChannel : volume) {
		volumeChannel = pow(10, db / 20.0);
	}
}

void FilterChain::setEqualizer(size_t index, EqFilter::FilterType type, double f0, double gain, double Q) {
	if(index < eqFilters.size()) {
		eqFilters[index].first = type != EqFilter::FilterType::None;
		for(EqFilter& eqFilter : eqFilters[index].second) {
			eqFilter.setParameters(type, f0, gain, Q);
		}
	}
}

void FilterChain::setDelay(double delay) {
	for(DelayFilter& delayFilter : delayFilters) {
		delayFilter.setParameters(delay);
	}
}

double FilterChain::getVolumeDb() {
	return 20.0 * log10(volume[0]);
}

bool FilterChain::getEqualizer(size_t index, EqFilter::FilterType& type, double& f0, double& gain, double& Q) {
	if(index < eqFilters.size()) {
		eqFilters[index].second[0].getParameters(type, f0, gain, Q);
		return true;
	} else {
		return false;
	}
}

double FilterChain::getDelay() {
	double delay;
	delayFilters[0].getParameters(delay);
	return delay;
}
