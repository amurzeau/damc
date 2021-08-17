#ifndef REVERBFILTER_H
#define REVERBFILTER_H

#include "../OscAddress.h"
#include "../json.h"
#include "DelayFilter.h"
#include <stddef.h>
#include <vector>

class ReverbFilter : public OscContainer {
public:
	ReverbFilter(OscContainer* parent, const std::string& name);
	void reset(int depth = 1, unsigned int innerReverberatorCount = 5);
	void processSamples(float* output, const float* input, size_t count);
	float processOneSample(float input);

	void setParameters(const nlohmann::json& json);
	nlohmann::json getParameters();

private:
	OscVariable<bool> enabled;
	OscVariable<int32_t> delay;
	DelayFilter delayFilter;
	OscVariable<float> gain;
	OscContainerArray<ReverbFilter> reverberators;

	float previousDelayOutput = 0.0f;
};

#endif
