#ifndef RESAMPLINGFILTER_H
#define RESAMPLINGFILTER_H

#include <array>
#include <stddef.h>
#include <vector>

class ResamplingFilter {
public:
	void reset(double fs);
	void put(double sample);
	int get(std::vector<float>& out, double period);

	int processSamples(std::vector<float>& output, const float* input, size_t count);
	int getNextOutputSize();
	size_t getMaxRequiredOutputSize(size_t count);
	size_t getMinRequiredOutputSize(size_t count);

	void setClockDrift(float drift);
	float getClockDrift();

	void setSourceSamplingRate(float samplingRate);
	float getSourceSamplingRate();

	void setTargetSamplingRate(float samplingRate);
	float getTargetSamplingRate();

	static constexpr unsigned int getOverSamplingRatio() { return oversamplingRatio; }
	float getDownSamplingRatio() { return downsamplingRatio; }

protected:
	inline double getLinearInterpolatedPoint(double delay) const;
	inline double getZeroOrderHoldInterpolatedPoint(double delay) const;
	inline double getOnePoint(unsigned int delay) const;

private:
	unsigned int currentPos;
	double previousDelay;
	std::array<double, 256> history;

	double baseSamplingRate = 48000.f;
	double targetSamplingRate = 48000.f;
	double downsamplingRatio = oversamplingRatio;

	static bool initialized;
	static const std::array<double, 8192> coefs;
	static std::array<double, 8192> optimized_coefs;
	static constexpr unsigned int oversamplingRatio = 128;
	static constexpr unsigned int tapPerSample = (unsigned int) coefs.size() / oversamplingRatio;
};

#endif
