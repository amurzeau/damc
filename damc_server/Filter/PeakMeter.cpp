#include "PeakMeter.h"
#include <OscRoot.h>
#include <algorithm>
#include <math.h>
#include <spdlog/spdlog.h>

PeakMeter::PeakMeter(OscContainer* parent,
                     OscReadOnlyVariable<int32_t>* oscNumChannel,
                     OscReadOnlyVariable<int32_t>* oscSampleRate)
    : oscRoot(parent->getRoot()),
      oscSampleRate(oscSampleRate),
      samplesInPeaks(0),
      oscEnablePeakUpdate(parent, "meter_enable_per_channel", false) {
	uv_mutex_init(&peakMutex);

	oscPeakGlobalPath = parent->getFullAddress() + "/meter";
	oscPeakPerChannelPath = parent->getFullAddress() + "/meter_per_channel";

	oscNumChannel->setChangeCallback([this](int32_t newValue) {
		levelsDb.resize(newValue, -192);

		uv_mutex_lock(&peakMutex);
		peaksPerChannel.resize(newValue, 0);
		uv_mutex_unlock(&peakMutex);
	});
}

PeakMeter::~PeakMeter() {
	uv_mutex_destroy(&peakMutex);
}

void PeakMeter::processSamples(const float* peaks, size_t numChannels, size_t samplesInPeaks) {
	uv_mutex_lock(&peakMutex);
	this->samplesInPeaks += samplesInPeaks;
	for(size_t i = 0; i < numChannels; i++) {
		this->peaksPerChannel[i] = fmaxf(peaks[i], this->peaksPerChannel[i]);
	}
	uv_mutex_unlock(&peakMutex);
}

void PeakMeter::onFastTimer() {
	int samples;
	int32_t sampleRate = oscSampleRate->get();
	std::vector<float> peaks(this->peaksPerChannel.size(), 0);

	uv_mutex_lock(&peakMutex);
	samples = this->samplesInPeaks;
	peaks.swap(this->peaksPerChannel);
	this->samplesInPeaks = 0;
	uv_mutex_unlock(&peakMutex);

	if(sampleRate == 0)
		return;

	float deltaT = (float) samples / sampleRate;
	float maxLevel = 0;

	for(size_t channel = 0; channel < peaks.size(); channel++) {
		float peakDb = peaks[channel] != 0 ? 20.0 * log10(peaks[channel]) : -INFINITY;

		float decayAmount = 11.76470588235294 * deltaT;  // -20dB / 1.7s
		float levelDb = std::max(levelsDb[channel] - decayAmount, peakDb);
		levelsDb[channel] = levelDb > -192 ? levelDb : -192;
		if(channel == 0 || levelsDb[channel] > maxLevel)
			maxLevel = levelsDb[channel];
	}

	if(oscEnablePeakUpdate.get()) {
		OscArgument argument = maxLevel;
		oscRoot->sendMessage(oscPeakGlobalPath, &argument, 1);
	}

	oscPeakPerChannelArguments.clear();
	oscPeakPerChannelArguments.reserve(levelsDb.size());
	for(auto v : levelsDb) {
		oscPeakPerChannelArguments.emplace_back(v);
	}
	oscRoot->sendMessage(oscPeakPerChannelPath, oscPeakPerChannelArguments.data(), oscPeakPerChannelArguments.size());
}
