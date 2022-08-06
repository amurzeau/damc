#pragma once

#include <Osc/OscVariable.h>
#include <mutex>
#include <stdint.h>
#include <string>
#include <mutex>
#include <vector>

class OscContainer;

class ControlInterface;

class PeakMeter {
public:
	PeakMeter(OscContainer* parent,
	          OscReadOnlyVariable<int32_t>* oscNumChannel,
	          OscReadOnlyVariable<int32_t>* oscSampleRate);
	~PeakMeter();

	void processSamples(const float* peaks, size_t numChannels, size_t samplesInPeaks);

	void onFastTimer();

private:
	OscRoot* oscRoot;
	OscReadOnlyVariable<int32_t>* oscSampleRate;

	std::vector<float> levelsDb;
	std::mutex peakMutex;
	int samplesInPeaks;
	std::vector<float> peaksPerChannel;
	std::string oscPeakGlobalPath;
	std::string oscPeakPerChannelPath;
	std::vector<OscArgument> oscPeakPerChannelArguments;

	OscVariable<bool> oscEnablePeakUpdate;
};
