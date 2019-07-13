#ifndef RESAMPLINGFILTER_H
#define RESAMPLINGFILTER_H

#include <array>
#include <stddef.h>
#include <vector>

class ResamplingFilter {
public:
	void reset();
	void put(double sample);
	int get(std::vector<float>& out, float period);

	int processSamples(std::vector<float>& output, const float* input, size_t count);
	int getNextOutputSize();

	void setClockDrift(float drift);
	float getClockDrift();

	void setSamplingRate(float samplingRate);

	static constexpr unsigned int getOverSamplingRatio() { return oversamplingRatio; }

protected:
	inline double getLinearInterpolatedPoint(float delay) const;
	inline double getZeroOrderHoldInterpolatedPoint(float delay) const;
	inline double getOnePoint(unsigned int delay) const;

private:
	unsigned int currentPos;
	float previousDelay;
	std::array<double, 256> history;

	float downsamplingRatio = oversamplingRatio;

	static bool initialized;
	static const std::array<double, 8192> coefs;
	static std::array<double, 8192> optimized_coefs;
	static constexpr unsigned int oversamplingRatio = 128;
	static constexpr unsigned int tapPerSample = (unsigned int) coefs.size() / oversamplingRatio;
};

#endif
