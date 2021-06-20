#include "ExpanderFilter.h"

#include <math.h>
#include <string.h>

void ExpanderFilter::init(size_t numChannel) {
	this->numChannel = numChannel;
	previousPartialGainComputerOutput.resize(numChannel);
	previousLevelDetectorOutput.resize(numChannel);
}

void ExpanderFilter::reset(double fs) {
	this->fs = fs;
	std::fill_n(previousPartialGainComputerOutput.begin(), numChannel, 0);
	std::fill_n(previousLevelDetectorOutput.begin(), numChannel, 0);
}

void ExpanderFilter::processSamples(float** output, const float** input, size_t count) {
	if(enable) {
		for(size_t i = 0; i < count; i++) {
			float lowestCompressionDb = -INFINITY;

			for(size_t channel = 0; channel < numChannel; channel++) {
				float dbGain = doCompression(input[channel][i],
				                             previousPartialGainComputerOutput[channel],
				                             previousLevelDetectorOutput[channel]);
				if(dbGain > lowestCompressionDb)
					lowestCompressionDb = dbGain;
			}

			// db to ratio
			float largerCompressionRatio;

			if(lowestCompressionDb != -INFINITY)
				largerCompressionRatio = powf(10, (lowestCompressionDb + makeUpGain) / 20);
			else
				largerCompressionRatio = 0;

			for(size_t channel = 0; channel < numChannel; channel++) {
				float value = largerCompressionRatio * input[channel][i];

				output[channel][i] = value;
			}
		}
	} else if(output != input) {
		for(size_t channel = 0; channel < numChannel; channel++) {
			std::copy_n(input[channel], count, output[channel]);
		}
	}
}

float ExpanderFilter::doCompression(float sample, float& y1, float& yL) {
	if(sample == 0)
		return -INFINITY;

	float dbSample = 20 * log10f(fabsf(sample));
	levelDetector(gainComputer(dbSample), y1, yL);
	return -yL;
}

float ExpanderFilter::gainComputer(float dbSample) {
	float zone = 2 * (dbSample - threshold);
	if(zone == -INFINITY || zone <= -kneeWidth) {
		return gainDiffRatio * (threshold - dbSample);
	} else if(zone >= kneeWidth) {
		return 0;
	} else {
		float a = threshold - dbSample + kneeWidth / 2;
		return gainDiffRatio * (a * a) / (2 * kneeWidth);
	}
}

void ExpanderFilter::levelDetector(float dbSample, float& y1, float& yL) {
	y1 = fminf(dbSample, alphaR * y1 + (1 - alphaR) * dbSample);
	yL = alphaA * yL + (1 - alphaA) * y1;
}

void ExpanderFilter::setParameters(const nlohmann::json& json) {
	enable = json.at("enabled").get<bool>();

	if(json.at("attackTime").get<float>() != 0)
		alphaA = expf(-1 / (json.at("attackTime").get<float>() * fs));
	else
		alphaA = 0;

	if(json.at("releaseTime").get<float>() != 0)
		alphaR = expf(-1 / (json.at("releaseTime").get<float>() * fs));
	else
		alphaR = 0;

	threshold = json.at("threshold").get<float>();
	makeUpGain = json.at("makeUpGain").get<float>();
	gainDiffRatio = json.at("ratio").get<float>() - 1;
	kneeWidth = json.at("kneeWidth").get<float>();
}

nlohmann::json ExpanderFilter::getParameters() {
	float attackTime, releaseTime;

	if(alphaA != 0)
		attackTime = -1 / logf(alphaA) / fs;
	else
		attackTime = 0;

	if(alphaR != 0)
		releaseTime = -1 / logf(alphaR) / fs;
	else
		releaseTime = 0;

	return nlohmann::json::object({{"enabled", enable},
	                               {"attackTime", attackTime},
	                               {"releaseTime", releaseTime},
	                               {"threshold", threshold},
	                               {"makeUpGain", makeUpGain},
	                               {"ratio", gainDiffRatio + 1},
	                               {"kneeWidth", kneeWidth}});
}
