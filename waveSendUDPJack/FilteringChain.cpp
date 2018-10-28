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

void FilterChain::setParameters(const nlohmann::json& json) {
	if(!json.is_object()) {
		printf("FilteringChain require json object, got %s\n", json.type_name());
		return;
	}

	auto mute = json.find("mute");
	if(mute != json.end()) {
		this->mute = mute.value().get<bool>();
	}

	auto volume = json.find("volume");
	if(volume != json.end()) {
		setVolumeDb(volume.value().get<double>());
	}

	auto delayFilter = json.find("delayFilter");
	if(delayFilter != json.end()) {
		for(DelayFilter& filter : delayFilters) {
			filter.setParameters(delayFilter.value());
		}
	}

	auto eqFiltersJson = json.find("eqFilters");
	if(eqFiltersJson != json.end()) {
		auto eqFiltersArray = eqFiltersJson.value();
		if(!eqFiltersArray.is_array()) {
			printf("eqFilter require json array, got %s\n", eqFiltersArray.type_name());
			return;
		}

		for(auto eqFilterJson : eqFiltersArray) {
			int index = eqFilterJson.at("index").get<int>();
			if(index >= 0 && index < (int) this->eqFilters.size()) {
				EqFilter::FilterType type = static_cast<EqFilter::FilterType>(eqFilterJson.at("type").get<int>());
				this->eqFilters[index].first = type != EqFilter::FilterType::None;
				for(EqFilter& eqFilter : this->eqFilters[index].second) {
					eqFilter.setParameters(eqFilterJson);
				}
			}
		}
	}
}

nlohmann::json FilterChain::getParameters() {
	nlohmann::json json = nlohmann::json::object(
	    {{"mute", this->mute}, {"volume", getVolumeDb()}, {"delayFilter", this->delayFilters[0].getParameters()}});
	json["eqFilters"] = nlohmann::json::array();

	for(size_t i = 0; i < eqFilters.size(); i++) {
		std::pair<bool, std::vector<EqFilter>>& filter = eqFilters[i];
		nlohmann::json eqFilterJson = filter.second[0].getParameters();
		eqFilterJson["index"] = i;
		json["eqFilters"].push_back(eqFilterJson);
	}

	return json;
}
