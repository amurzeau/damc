#ifndef COMPRESSORFILTER_H
#define COMPRESSORFILTER_H

#include <Osc/OscContainer.h>
#include <Osc/OscVariable.h>
#include <array>
#include <deque>
#include <stddef.h>
#include <vector>

class CompressorFilter : public OscContainer {
protected:
	struct PerChannelData {
		float y1;
		float yL;

		size_t compressionHistoryPtr;
		std::array<float, 1024> compressionHistory;
		std::deque<size_t> compressionMovingMaxDeque;

		float speed;

		float movingMax(float dbCompression);
		float noProcessing(float dbCompression);
	};

public:
	CompressorFilter(OscContainer* parent);
	void init(size_t numChannel);
	void reset(double fs);
	void processSamples(float** output, const float** input, size_t count);

protected:
	float doCompression(float sample, PerChannelData& perChannelData);
	float gainComputer(float sample) const;
	void levelDetector(float sample, PerChannelData& perChannelData);

private:
	size_t numChannel;
	std::vector<PerChannelData> perChannelData;

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
	uint32_t gainHoldSamples = 48000 / 20;  // 20Hz period
	OscVariable<bool> useMovingMax;

	static const float LOG10_VALUE_DIV_20;
};

#endif
