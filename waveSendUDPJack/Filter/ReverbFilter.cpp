#include "ReverbFilter.h"

#include <algorithm>
#include <math.h>
#include <string.h>

ReverbFilter::ReverbFilter(OscContainer* parent, const std::string& name)
    : OscContainer(parent, name),
      enabled(this, "enabled", false),
      delay(this, "delay", 1440),
      gain(this, "gain", 0.893),
      reverberators(this, "innerReverberators") {
	delay.setChangeCallback([this](int32_t oscValue) { delayFilter.setParameters(oscValue); });
}

void ReverbFilter::reset(int depth, unsigned int innerReverberatorCount) {
	this->delayFilter.reset();

	if(depth > 0) {
		reverberators.resize(innerReverberatorCount);
		for(auto& filter : reverberators)
			filter->reset(depth - 1, innerReverberatorCount);
	}
}

void ReverbFilter::processSamples(float* output, const float* input, size_t count) {
	if(enabled) {
		for(size_t i = 0; i < count; i++) {
			output[i] = processOneSample(input[i]);
		}
	}
}

float ReverbFilter::processOneSample(float input) {
	float delayedSample = delayFilter.processOneSample(input + previousDelayOutput * gain);

	for(auto& reverberator : reverberators)
		delayedSample = reverberator->processOneSample(delayedSample);

	previousDelayOutput = delayedSample;

	float gain = this->gain;
	return -gain * input + (1 - (gain * gain)) * delayedSample;
}

void ReverbFilter::setParameters(const nlohmann::json& json) {
	delay = json["delay"].get<unsigned int>();
	gain = json["gain"].get<float>();

	auto innerReverberatorsJson = json.find("innerReverberators");
	if(innerReverberatorsJson != json.end() && innerReverberatorsJson->is_array()) {
		size_t i = 0;
		for(auto& reverberatorJson : innerReverberatorsJson.value()) {
			if(i < reverberators.size())
				reverberators[i].setParameters(reverberatorJson);
			i++;
		}
	}
}

nlohmann::json ReverbFilter::getParameters() {
	std::vector<nlohmann::json> reverberatorsJson;
	std::transform(reverberators.begin(),
	               reverberators.end(),
	               std::back_inserter(reverberatorsJson),
	               [](std::unique_ptr<ReverbFilter>& filter) { return filter->getParameters(); });

	nlohmann::json ret = nlohmann::json::object({{"enabled", enabled.get()},
	                                             {"delay", delay.get()},
	                                             {"gain", gain.get()},
	                                             {"innerReverberators", reverberatorsJson}});

	return ret;
}
