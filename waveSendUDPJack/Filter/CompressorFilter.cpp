#include "CompressorFilter.h"

#include <math.h>
#include <string.h>

void CompressorFilter::init(size_t numChannel) {
	this->numChannel = numChannel;
	previousPartialGainComputerOutput.resize(numChannel);
	previousLevelDetectorOutput.resize(numChannel);
}

void CompressorFilter::reset() {
	std::fill_n(previousPartialGainComputerOutput.begin(), numChannel, 0);
	std::fill_n(previousLevelDetectorOutput.begin(), numChannel, 0);
}

void CompressorFilter::processSamples(float** output, const float** input, size_t count) {
	if(enable) {
		for(size_t i = 0; i < count; i++) {
			float largerCompressionDb = 0;

			for(size_t channel = 0; channel < numChannel; channel++) {
				float dbGain = doCompression(input[channel][i],
				                             previousPartialGainComputerOutput[channel],
				                             previousLevelDetectorOutput[channel]);
				if(dbGain < largerCompressionDb)
					largerCompressionDb = dbGain;
			}

			// db to ratio
			float largerCompressionRatio = powf(10, (largerCompressionDb + makeUpGain) / 20);

			for(size_t channel = 0; channel < numChannel; channel++) {
				output[channel][i] = largerCompressionRatio * input[channel][i];
			}
		}
	} else if(output != input) {
		for(size_t channel = 0; channel < numChannel; channel++) {
			std::copy_n(input[channel], count, output[channel]);
		}
	}
}

float CompressorFilter::doCompression(float sample, float& y1, float& yL) {
	if(sample == 0)
		return 0;

	float dbSample = 20 * log10f(fabsf(sample));
	levelDetector(gainComputer(dbSample), y1, yL);
	return -yL;
}

float CompressorFilter::gainComputer(float dbSample) {
	float zone = 2 * (dbSample - threshold);
	if(zone == -INFINITY || zone <= -kneeWidth) {
		return 0;
	} else if(zone >= kneeWidth) {
		return gainDiffRatio * (dbSample - threshold);
	} else {
		float a = dbSample - threshold + kneeWidth / 2;
		return gainDiffRatio * (a * a) / (2 * kneeWidth);
	}
}

void CompressorFilter::levelDetector(float dbSample, float& y1, float& yL) {
	y1 = fmaxf(dbSample, alphaR * y1 + (1 - alphaR) * dbSample);
	yL = alphaA * yL + (1 - alphaA) * y1;
}

void CompressorFilter::setParameters(const nlohmann::json& json) {
	enable = json.at("enabled").get<bool>();

	if(json.at("attackTime").get<float>() != 0)
		alphaA = expf(-1 / (json.at("attackTime").get<float>() * 48000.0f));
	else
		alphaA = 0;

	if(json.at("releaseTime").get<float>() != 0)
		alphaR = expf(-1 / (json.at("releaseTime").get<float>() * 48000.0f));
	else
		alphaR = 0;

	threshold = json.at("threshold").get<float>();
	makeUpGain = json.at("makeUpGain").get<float>();
	gainDiffRatio = 1 - 1 / json.at("compressionRatio").get<float>();
	kneeWidth = json.at("kneeWidth").get<float>();
}

nlohmann::json CompressorFilter::getParameters() {
	float attackTime, releaseTime;

	if(alphaA != 0)
		attackTime = -1 / logf(alphaA) / 48000.0f;
	else
		attackTime = 0;

	if(alphaR != 0)
		releaseTime = -1 / logf(alphaR) / 48000.0f;
	else
		releaseTime = 0;

	return nlohmann::json::object({{"enabled", enable},
	                               {"attackTime", attackTime},
	                               {"releaseTime", releaseTime},
	                               {"threshold", threshold},
	                               {"makeUpGain", makeUpGain},
	                               {"compressionRatio", 1 / (1 - gainDiffRatio)},
	                               {"kneeWidth", kneeWidth}});
}
