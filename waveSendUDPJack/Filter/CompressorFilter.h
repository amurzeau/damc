#ifndef COMPRESSORFILTER_H
#define COMPRESSORFILTER_H

#include "../json.h"
#include <array>
#include <deque>
#include <stddef.h>
#include <vector>

class CompressorFilter {
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
	void init(size_t numChannel);
	void reset();
	void processSamples(float** output, const float** input, size_t count);

	void setParameters(const nlohmann::json& json);
	nlohmann::json getParameters();

protected:
	float doCompression(float sample, PerChannelData& perChannelData);
	float gainComputer(float sample) const;
	void levelDetector(float sample, PerChannelData& perChannelData);

private:
	size_t numChannel;
	std::vector<PerChannelData> perChannelData;

	bool enable = false;
	float alphaR = 0.99916701379245836213502440855751;
	float alphaA = 0.99916701379245836213502440855751;
	float threshold = 0;
	float makeUpGain = 0;
	float gainDiffRatio = 0;
	float kneeWidth = 0;
	uint32_t gainHoldSamples = 48000 / 20;  // 20Hz period
	bool useMovingMax = true;

	static const float LOG10_VALUE_DIV_20;
};

#endif
