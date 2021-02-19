#include "ReverbFilter.h"

#include <algorithm>
#include <math.h>
#include <string.h>

ReverbFilter::ReverbFilter() {
	this->delayFilter.setParameters(1440);
}

void ReverbFilter::reset(int depth, unsigned int innerReverberatorCount) {
	this->delayFilter.reset();

	if(depth > 0) {
		reverberators.resize(innerReverberatorCount);
		for(ReverbFilter& filter : reverberators)
			filter.reset(depth - 1, innerReverberatorCount);
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

	for(ReverbFilter& reverberator : reverberators)
		delayedSample = reverberator.processOneSample(delayedSample);

	previousDelayOutput = delayedSample;

	return -gain * input + (1 - (gain * gain)) * delayedSample;
}

void ReverbFilter::setParameters(const nlohmann::json& json) {
	delayFilter.setParameters(json["delay"].get<unsigned int>());
	gain = json["gain"].get<float>();

	auto innerReverberatorsJson = json.find("innerReverberators");
	if(innerReverberatorsJson != json.end() && innerReverberatorsJson->is_array()) {
		size_t i = 0;
		for(auto reverberatorJson : innerReverberatorsJson.value()) {
			if(i < reverberators.size())
				reverberators[i].setParameters(reverberatorJson);
			i++;
		}
	}
}

nlohmann::json ReverbFilter::getParameters() {
	unsigned int delay;
	delayFilter.getParameters(delay);

	std::vector<nlohmann::json> reverberatorsJson;
	std::transform(
	    reverberators.begin(), reverberators.end(), std::back_inserter(reverberatorsJson), [](ReverbFilter& filter) {
		    return filter.getParameters();
	    });

	nlohmann::json ret = nlohmann::json::object(
	    {{"enabled", enabled}, {"delay", delay}, {"gain", gain}, {"innerReverberators", reverberatorsJson}});

	return ret;
}
