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
	reverberators.setFactory(
	    [](OscContainer* parent, int name) { return new ReverbFilter(parent, std::to_string(name)); });
	delay.addChangeCallback([this](int32_t oscValue) { delayFilter.setParameters(oscValue); });
}

void ReverbFilter::reset(int depth, unsigned int innerReverberatorCount) {
	this->delayFilter.reset();

	if(depth > 0) {
		reverberators.resize(innerReverberatorCount);
		for(auto& filter : reverberators)
			filter.second->reset(depth - 1, innerReverberatorCount);
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
		delayedSample = reverberator.second->processOneSample(delayedSample);

	previousDelayOutput = delayedSample;

	float gain = this->gain;
	return -gain * input + (1 - (gain * gain)) * delayedSample;
}
