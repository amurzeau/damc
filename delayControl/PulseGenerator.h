#ifndef PULSEGENERATOR_H

#include <atomic>
#include <stdint.h>
#include <string>
#include <vector>

class ServerControl;

class PulseGenerator {
public:
	PulseGenerator(const std::vector<float>& pulseData,
	               size_t delay,
	               ServerControl* serverControl,
	               int outputInstance,
	               float sampleRate);

	void processSamples(float* buffer, uint32_t nframes);
	void startNewPulse();

	void computeDelayControl(int delayError, uint64_t timestamp);

private:
	float sampleRate;
	ServerControl* serverControl;
	int outputInstance;

	std::vector<float> pulseWave;
	size_t delay;

	ssize_t currentPos;

	// PI control delay
	double smoothedError;
	double previousError;
	uint64_t previousTime;
	double integralError;
	double sampleRateControl;

	int previousBigDelayError[3];
};

#endif
