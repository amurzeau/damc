#ifndef EXPANDERFILTER_H
#define EXPANDERFILTER_H

#include "../OscAddress.h"
#include "../json.h"
#include <stddef.h>
#include <vector>

class ExpanderFilter : public OscContainer {
public:
	ExpanderFilter(OscContainer* parent);
	void init(size_t numChannel);
	void reset(double fs);
	void processSamples(float** output, const float** input, size_t count);

	void setParameters(const nlohmann::json& json);
	nlohmann::json getParameters();

protected:
	float doCompression(float sample, float& y1, float& yL);
	float gainComputer(float sample);
	void levelDetector(float sample, float& y1, float& yL);

private:
	size_t numChannel;
	std::vector<float> previousPartialGainComputerOutput;
	std::vector<float> previousLevelDetectorOutput;

	OscVariable<bool> enable;
	double fs = 48000;
	float alphaR;
	float alphaA;
	OscVariable<float> attackTime;
	OscVariable<float> releaseTime;
	OscVariable<float> threshold;
	OscVariable<float> makeUpGain;
	OscVariable<float> ratio;
	float gainDiffRatio = 0;
	OscVariable<float> kneeWidth;
};

#endif
