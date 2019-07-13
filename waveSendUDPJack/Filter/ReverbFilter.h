#ifndef REVERBFILTER_H
#define REVERBFILTER_H

#include "../json.h"
#include "DelayFilter.h"
#include <stddef.h>
#include <vector>

class ReverbFilter {
public:
	ReverbFilter();
	void reset(int depth = 1, unsigned int innerReverberatorCount = 5);
	void processSamples(float* output, const float* input, size_t count);
	float processOneSample(float input);

	void setParameters(const nlohmann::json& json);
	nlohmann::json getParameters();

private:
	bool enabled;
	DelayFilter delayFilter;
	float gain;
	std::vector<ReverbFilter> reverberators;

	float previousDelayOutput;
};

#endif
