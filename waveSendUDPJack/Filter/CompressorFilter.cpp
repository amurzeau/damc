#include "CompressorFilter.h"

#include <math.h>
#include <string.h>

const float CompressorFilter::LOG10_VALUE_DIV_20 = std::log(10) / 20;

void CompressorFilter::init(size_t numChannel) {
	this->numChannel = numChannel;
	perChannelData.resize(numChannel);
}

void CompressorFilter::reset() {
	std::fill_n(perChannelData.begin(), numChannel, PerChannelData{});
}

void CompressorFilter::processSamples(float** output, const float** input, size_t count) {
	if(enable) {
		float staticGain = gainComputer(0) + makeUpGain;
		for(size_t i = 0; i < count; i++) {
			float largerCompressionDb = 0;
			for(size_t channel = 0; channel < numChannel; channel++) {
				float dbGain = doCompression(input[channel][i], perChannelData[channel]);
				if(dbGain < largerCompressionDb)
					largerCompressionDb = dbGain;
			}

			// db to ratio
			float largerCompressionRatio = expf(LOG10_VALUE_DIV_20 * (largerCompressionDb + staticGain));

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

float CompressorFilter::doCompression(float sample, PerChannelData& perChannelData) {
	if(sample == 0)
		return 0;

	float dbSample = logf(fabsf(sample)) / LOG10_VALUE_DIV_20;
	levelDetector(gainComputer(dbSample), perChannelData);
	return -perChannelData.yL;
}

float CompressorFilter::gainComputer(float dbSample) const {
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

float CompressorFilter::PerChannelData::movingMax(float dbCompression) {
	if(!compressionMovingMaxDeque.empty() && compressionMovingMaxDeque.front() == compressionHistoryPtr)
		compressionMovingMaxDeque.pop_front();

	while(!compressionMovingMaxDeque.empty() && dbCompression >= compressionHistory[compressionMovingMaxDeque.back()])
		compressionMovingMaxDeque.pop_back();

	compressionMovingMaxDeque.push_back(compressionHistoryPtr);
	compressionHistory[compressionHistoryPtr] = dbCompression;
	compressionHistoryPtr = (compressionHistoryPtr + 1) % compressionHistory.size();

	return compressionHistory[compressionMovingMaxDeque.front()];
}

float CompressorFilter::PerChannelData::noProcessing(float dbCompression) {
	return dbCompression;
}

void CompressorFilter::levelDetector(float dbCompression, PerChannelData& perChannelData) {
	float decayedCompression = alphaR * perChannelData.y1 + (1 - alphaR) * dbCompression;
	if(useMovingMax)
		perChannelData.y1 = fmaxf(perChannelData.movingMax(dbCompression), decayedCompression);
	else
		perChannelData.y1 = fmaxf(dbCompression, decayedCompression);
	perChannelData.yL = alphaA * perChannelData.yL + (1 - alphaA) * perChannelData.y1;
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
	gainDiffRatio = 1 - 1 / json.at("ratio").get<float>();
	kneeWidth = json.at("kneeWidth").get<float>();
	useMovingMax = json.value("useMovingMax", true);
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
	                               {"ratio", 1 / (1 - gainDiffRatio)},
	                               {"kneeWidth", kneeWidth},
	                               {"useMovingMax", useMovingMax}});
}
