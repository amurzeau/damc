#include "FilteringChain.h"
#include <math.h>
#include <string.h>

FilterChain::FilterChain() {}

void FilterChain::init(size_t numChannel) {
	delayFilters.resize(numChannel + 1);  // +1 for side channel
	reverbFilters.resize(numChannel);
	volume.resize(numChannel, 1);

	eqFilters.resize(6);

	for(EqFilter& filter : eqFilters) {
		filter.init(numChannel);
	}

	compressorFilter.init(numChannel);
	expanderFilter.init(numChannel);
}

void FilterChain::reset(double fs) {
	for(DelayFilter& delayFilter : delayFilters) {
		delayFilter.reset();
	}
	for(ReverbFilter& reverbFilter : reverbFilters) {
		reverbFilter.reset();
	}

	for(EqFilter& filter : eqFilters) {
		filter.reset(fs);
	}

	compressorFilter.reset();
	expanderFilter.reset();
}

void FilterChain::processSamples(
    float* peakOutput, float** output, const float** input, size_t numChannel, size_t count) {
	for(uint32_t channel = 0; channel < numChannel; channel++) {
		delayFilters[channel].processSamples(output[channel], input[channel], count);
		reverbFilters[channel].processSamples(output[channel], output[channel], count);
	}

	for(EqFilter& filter : eqFilters) {
		filter.processSamples(output, const_cast<const float**>(output), count);
	}

	if(!mute) {
		expanderFilter.processSamples(output, const_cast<const float**>(output), count);
		compressorFilter.processSamples(output, const_cast<const float**>(output), count);
	}

	for(uint32_t channel = 0; channel < numChannel; channel++) {
		float volume = this->volume[channel] * this->masterVolume;
		float peak = 0;
		for(size_t i = 0; i < count; i++) {
			output[channel][i] *= volume;
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
			if(index >= 0) {
				while(index >= (int) this->eqFilters.size()) {
					this->eqFilters.emplace_back();
					this->eqFilters.back().init(this->volume.size());
				}
				this->eqFilters[index].setParameters(eqFilterJson);
			}
		}
	}

	auto compressorFilterJson = json.find("compressorFilter");
	if(compressorFilterJson != json.end()) {
		compressorFilter.setParameters(compressorFilterJson.value());
	}

	auto expanderFilterJson = json.find("expanderFilter");
	if(expanderFilterJson != json.end()) {
		expanderFilter.setParameters(expanderFilterJson.value());
	}

	auto reverbFilterJson = json.find("reverbFilter");
	if(reverbFilterJson != json.end() && reverbFilterJson->is_array()) {
		size_t i = 0;
		for(const auto& reverbFilterItem : reverbFilterJson.value()) {
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
	                                              {"compressorFilter", this->compressorFilter.getParameters()},
	                                              {"expanderFilter", this->expanderFilter.getParameters()}});
	json["eqFilters"] = nlohmann::json::array();

	for(size_t i = 0; i < eqFilters.size(); i++) {
		EqFilter& filter = eqFilters[i];
		nlohmann::json eqFilterJson = filter.getParameters();
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
