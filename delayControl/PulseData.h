#ifndef PULSEDATA_H

#include "../waveSendUDPJack/OutputInstance/ResamplingFilter.h"
#include <array>
#include <string>
#include <uv.h>
#include <vector>

#include <jack/jack.h>

class BiquadFilter {
public:
	void init(const double a_coefs[3], const double b_coefs[3]);
	void update(const double a_coefs[3], const double b_coefs[3]);
	double put(double input);

public:
	// Use offset of 0.5 to avoid denormals
	double s1 = 0;
	double s2 = 0;
	double b_coefs[3] = {};
	double a_coefs[2] = {};
};

class PulseData {
public:
	int open(const std::string& pulseFilename, jack_nframes_t bufferSize, float sampleRate);

	const std::vector<jack_default_audio_sample_t>& getData() { return pulseWave; }

	void doCrossCorrelation(jack_default_audio_sample_t* in,
	                        jack_default_audio_sample_t* out1,
	                        jack_default_audio_sample_t* out2,
	                        jack_default_audio_sample_t* out3,
	                        jack_nframes_t nframes);

	std::vector<uint64_t> retrieveDetectedPulses();

protected:
	jack_default_audio_sample_t doCFAR(int delay);
	void doAGC(jack_default_audio_sample_t in,
	           jack_default_audio_sample_t& out1,
	           jack_default_audio_sample_t& out2,
	           jack_default_audio_sample_t& out3);

private:
	float sampleRate;
	std::vector<jack_default_audio_sample_t> pulseWave;
	std::array<jack_default_audio_sample_t, 8192> history;
	size_t currentHistoryPos;

	uint64_t previousPeak;
	uint64_t currentTime;
	float filteredHistory[2048];
	unsigned int currentFilteredHistoryPos;
	std::array<std::pair<float, float>, 16> peakHistory;
	unsigned int currentPeakHistoryPos;
	float previousSample = 0;
	float previousDerivative = 0;

	bool insidePeak;
	float snapshotedThreshold;

	ResamplingFilter resamplingFilter;
	uv_mutex_t detectedPulsesMutex;
	std::vector<uint64_t> detectedPulses;
};

#endif
