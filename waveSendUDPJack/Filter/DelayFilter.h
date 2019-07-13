#ifndef DELAYFILTER_H
#define DELAYFILTER_H

#include "../json.h"
#include <stddef.h>
#include <vector>

class DelayFilter {
public:
	DelayFilter();

	void reset();
	void processSamples(float* output, const float* input, size_t count);
	float processOneSample(float input);

	void setParameters(unsigned int delay);
	void getParameters(unsigned int& delay) { delay = this->delay; }

	void setParameters(const nlohmann::json& json) { setParameters(json["delay"].get<unsigned int>()); }
	nlohmann::json getParameters() { return nlohmann::json::object({{"delay", delay}}); }

private:
	std::vector<float> delayedSamples;
	unsigned int delay;
	size_t inputIndex;
	size_t outputIndex;
	size_t power2Size;
};

#endif
