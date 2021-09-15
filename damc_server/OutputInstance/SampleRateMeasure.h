#pragma once

#include <Osc/OscContainer.h>
#include <Osc/OscDynamicVariable.h>
#include <atomic>
#include <memory.h>
#include <stdint.h>
#include <vector>

class SampleRateMeasure {
public:
	SampleRateMeasure(OscContainer* oscParent, const std::string& name);
	void notifySampleProcessed(size_t nframes);

	void onTimeoutTimer();

private:
	struct TimingInfo {
		uint64_t firstSampleTime = 0;
		uint64_t lastSampleTime = 0;
		size_t sampleNumber = 0;
	};
	TimingInfo data;

	std::atomic<TimingInfo*> runningData;

	OscDynamicVariable<float> oscRealSampleRate;
};
