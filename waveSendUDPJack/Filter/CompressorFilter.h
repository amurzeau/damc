#ifndef COMPRESSORFILTER_H
#define COMPRESSORFILTER_H

#include "../json.h"
#include <stddef.h>
#include <vector>

class CompressorFilter {
public:
	void init(size_t numChannel);
	void reset();
	void processSamples(float** output, const float** input, size_t count);

	void setParameters(const nlohmann::json& json);
	nlohmann::json getParameters();

protected:
	float doCompression(float sample, float& y1, float& yL);
	float gainComputer(float sample) const;
	void levelDetector(float sample, float& y1, float& yL);

private:
	size_t numChannel;
	std::vector<float> previousPartialGainComputerOutput;
	std::vector<float> previousLevelDetectorOutput;

	bool enable = false;
	float alphaR = 0.99916701379245836213502440855751;
	float alphaA = 0.99916701379245836213502440855751;
	float threshold = 0;
	float makeUpGain = 0;
	float gainDiffRatio = 0;
	float kneeWidth = 0;

	static const float LOG10_VALUE_DIV_20;
};

#endif
