#include "PulseGenerator.h"
#include "ServerControl.h"
#include <algorithm>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <string>

PulseGenerator::PulseGenerator(const std::vector<float>& pulseData,
                               size_t delay,
                               ServerControl* serverControl,
                               int outputInstance,
                               float sampleRate)
    : sampleRate(sampleRate),
      serverControl(serverControl),
      outputInstance(outputInstance),
      delay(delay),
      smoothedError(0),
      previousError(0),
      previousTime(0),
      integralError(1),
      sampleRateControl(1),
      previousBigDelayError{0} {
	pulseWave = pulseData;
	std::for_each(pulseWave.begin(), pulseWave.end(), [](float& sample) { sample *= powf(10, -20 / 20); });

	this->currentPos = -(ssize_t(delay));

	printf("Pulse generator with delay %f ms\n", delay / sampleRate * 1000);
}

void PulseGenerator::processSamples(float* buffer, uint32_t nframes) {
	size_t bufferPos = 0;

	if(currentPos < 0) {
		if(currentPos + ssize_t(nframes) <= 0) {
			currentPos += ssize_t(nframes);
			bufferPos = nframes;
		} else {
			bufferPos = -currentPos;
			currentPos = 0;
		}
		memset(buffer, 0, nframes * sizeof(buffer[0]));
	}

	// printf("nframes: %u, bufferPos: %zu, currentPos: %d\n", nframes, bufferPos, currentPos);

	for(; bufferPos < nframes && currentPos < (ssize_t) pulseWave.size();) {
		size_t chunkSize = nframes - bufferPos;
		if(chunkSize > pulseWave.size() - currentPos)
			chunkSize = pulseWave.size() - currentPos;

		std::copy(pulseWave.begin() + currentPos, pulseWave.begin() + currentPos + chunkSize, buffer + bufferPos);

		bufferPos += chunkSize;
		currentPos += chunkSize;
	}

	std::fill_n(buffer + bufferPos, nframes - bufferPos, 0);
}

void PulseGenerator::startNewPulse() {
	this->currentPos = -(ssize_t(delay));
}

void PulseGenerator::computeDelayControl(int delayError, uint64_t timestamp) {
	if(abs(delayError) > 96) {
		size_t i;
		bool tooBigDiff = false;
		int lastError = 0;
		static const size_t PREVIOUS_BIG_DELAY_ERROR_SIZE =
		    sizeof(previousBigDelayError) / sizeof(previousBigDelayError[0]);

		for(i = 0; i < PREVIOUS_BIG_DELAY_ERROR_SIZE; i++) {
			if(previousBigDelayError[i] == 0) {
				previousBigDelayError[i] = delayError;
				break;
			} else if(abs(previousBigDelayError[i] - delayError) >= 15) {
				// Too big difference, not stable
				// reset
				tooBigDiff = true;
				lastError = abs(previousBigDelayError[i] - delayError);
				memset(previousBigDelayError, 0, sizeof(previousBigDelayError));
				break;
			}
		}

		printf("Delay error: %.3fms (%d), smooth: %fms (%.3f), drift: %.3f, reset: %d, error: %d\n",
		       delayError / (sampleRate / 1000),
		       delayError,
		       smoothedError / (sampleRate / 1000),
		       smoothedError,
		       sampleRateControl,
		       tooBigDiff,
		       lastError);

		if(i == PREVIOUS_BIG_DELAY_ERROR_SIZE) {
			printf("Too big delay on instance %d, adjusting %d\n", outputInstance, -delayError);
			serverControl->adjustDelay(outputInstance, -delayError);

			// Reset PID
			memset(previousBigDelayError, 0, sizeof(previousBigDelayError));
			smoothedError = previousError = 0;
			// integralError = 1;
			previousTime = 0;
			sampleRateControl = integralError;
		}
		return;
	} else {
		double timeDiff = (timestamp - previousTime) / sampleRate;
		smoothedError = 0.95 * smoothedError + 0.05 * delayError;
		double computedDrift = 1;

		// double derivativeError = (smoothedError - previousError) * sampleRate / (timestamp - previousTime);
		if(previousTime > 0) {
			double smoothedErrorDiff = (delayError - previousError) / sampleRate;
			computedDrift = sampleRateControl / (smoothedErrorDiff / timeDiff + 1);
			integralError = integralError * 0.98 + 0.02 * computedDrift;
			sampleRateControl = integralError;
		}

		previousTime = timestamp;
		previousError = delayError;

		// Integrate error on calculated drift (to remove noise)
		// Add an offset to compensate static delay (smoothedError)
		sampleRateControl = sampleRateControl * (1 - smoothedError / sampleRate / timeDiff / 10);

		serverControl->setClockDrift(outputInstance, sampleRateControl);

		printf("Delay error: %.3fms (%d), smooth: %fms (%.3f), drift: %.3f, integral: %.6f, instance: %d\n",
		       delayError / (sampleRate / 1000),
		       delayError,
		       smoothedError / (sampleRate / 1000),
		       smoothedError,
		       (computedDrift - 1) * 1000000,
		       integralError,
		       outputInstance);
		memset(previousBigDelayError, 0, sizeof(previousBigDelayError));
	}
}
