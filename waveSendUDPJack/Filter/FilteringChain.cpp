#include "FilteringChain.h"
#include <math.h>
#include <string.h>

FilterChain::FilterChain() {
	mute = false;
	enabled = true;
	eqFilters.resize(6, std::make_pair(false, std::vector<EqFilter>()));
}

void FilterChain::init(size_t numChannel) {
	delayFilters.resize(numChannel + 1);  // +1 for side channel
	reverbFilters.resize(numChannel);
	volume.resize(numChannel, 1);

	for(std::pair<bool, std::vector<EqFilter>>& filter : eqFilters) {
		filter.second.resize(numChannel);
	}

	compressorFilter.init(numChannel);
}

void FilterChain::reset(double fs) {
	for(DelayFilter& delayFilter : delayFilters) {
		delayFilter.reset();
	}
	for(ReverbFilter& reverbFilter : reverbFilters) {
		reverbFilter.reset();
	}

	for(std::pair<bool, std::vector<EqFilter>>& filter : eqFilters) {
		for(EqFilter& eqFilter : filter.second) {
			eqFilter.reset(fs);
		}
	}

	compressorFilter.reset();
}

void FilterChain::processSamples(
    float* peakOutput, float** output, const float** input, size_t numChannel, size_t count) {
	for(uint32_t channel = 0; channel < numChannel; channel++) {
		delayFilters[channel].processSamples(output[channel], input[channel], count);
		reverbFilters[channel].processSamples(output[channel], output[channel], count);

		for(std::pair<bool, std::vector<EqFilter>>& filter : eqFilters) {
			if(filter.first) {
				filter.second[channel].processSamples(output[channel], output[channel], count);
			}
		}

		float volume = this->volume[channel] * this->masterVolume;
		for(size_t i = 0; i < count; i++) {
			output[channel][i] *= volume;
		}
	}

	if(!mute) {
		compressorFilter.processSamples(output, const_cast<const float**>(output), count);
	}

	for(uint32_t channel = 0; channel < numChannel; channel++) {
		float peak = 0;
		for(size_t i = 0; i < count; i++) {
			if(fabsf(output[channel][i]) >= peak)
				peak = fabsf(output[channel][i]);
		}
		peakOutput[channel] = peak;
	}

	if(mute) {
		for(uint32_t channel = 0; channel < numChannel; channel++) {
			std::fill_n(output[channel], count, 0);
		}
	}
}

float FilterChain::processSideChannelSample(float input) {
	return delayFilters.back().processOneSample(input);
}

void FilterChain::setEqualizer(size_t index, EqFilter::FilterType type, double f0, double gain, double Q) {
	if(index < eqFilters.size()) {
		eqFilters[index].first = type != EqFilter::FilterType::None;
		for(EqFilter& eqFilter : eqFilters[index].second) {
			eqFilter.setParameters(type, f0, gain, Q);
		}
	}
}

bool FilterChain::getEqualizer(size_t index, EqFilter::FilterType& type, double& f0, double& gain, double& Q) {
	if(index < eqFilters.size()) {
		eqFilters[index].second[0].getParameters(type, f0, gain, Q);
		return true;
	} else {
		return false;
	}
}

void FilterChain::setParameters(const nlohmann::json& json) {
	if(!json.is_object()) {
		printf("FilteringChain require json object, got %s\n", json.type_name());
		return;
	}

	auto enabled = json.find("enabled");
	if(enabled != json.end()) {
		this->enabled = enabled.value().get<bool>();
	}

	auto mute = json.find("mute");
	if(mute != json.end()) {
		this->mute = mute.value().get<bool>();
	}

	auto volume = json.find("volume");
	if(volume != json.end()) {
		this->masterVolume = powf(10, volume.value().get<float>() / 20.0f);
	}

	auto balance = json.find("balance");
	if(balance != json.end() && balance->is_array()) {
		for(auto balanceItem : balance.value()) {
			int channel = balanceItem["channel"].get<int>();
			float balanceLevel = balanceItem["volume"].get<float>();
			if(channel < (int) this->volume.size())
				this->volume[channel] = powf(10, balanceLevel / 20.0f);
		}
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

	auto compressorFilterJson = json.find("compressorFilter");
	if(compressorFilterJson != json.end()) {
		compressorFilter.setParameters(compressorFilterJson.value());
	}

	auto reverbFilterJson = json.find("reverbFilter");
	if(reverbFilterJson != json.end() && reverbFilterJson->is_array()) {
		size_t i = 0;
		for(auto reverbFilterItem : reverbFilterJson.value()) {
			if(i < reverbFilters.size())
				reverbFilters[i].setParameters(reverbFilterItem);
			i++;
		}
	}
}

nlohmann::json FilterChain::getParameters() {
	nlohmann::json json = nlohmann::json::object({{"enabled", this->enabled},
	                                              {"mute", this->mute},
	                                              {"volume", 20.0 * log10(masterVolume)},
	                                              {"delayFilter", this->delayFilters[0].getParameters()},
	                                              {"compressorFilter", this->compressorFilter.getParameters()}});
	json["eqFilters"] = nlohmann::json::array();

	for(size_t i = 0; i < eqFilters.size(); i++) {
		std::pair<bool, std::vector<EqFilter>>& filter = eqFilters[i];
		nlohmann::json eqFilterJson = filter.second[0].getParameters();
		eqFilterJson["index"] = i;
		json["eqFilters"].push_back(eqFilterJson);
	}

	json["balance"] = nlohmann::json::array();

	for(size_t i = 0; i < this->volume.size(); i++) {
		nlohmann::json balanceItem;
		balanceItem["channel"] = i;
		balanceItem["volume"] = 20.0 * log10(volume[i]);
		json["balance"].push_back(balanceItem);
	}

	json["reverbFilter"] = nlohmann::json::array();
	for(ReverbFilter& filter : reverbFilters) {
		json["reverbFilter"].push_back(filter.getParameters());
	}

	return json;
}
