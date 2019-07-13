#ifndef DELAYMEASURE_H

#include "PulseData.h"
#include "PulseGenerator.h"
#include "ServerControl.h"
#include <atomic>
#include <string>
#include <uv.h>
#include <vector>

#include <jack/jack.h>

class DelayMeasure {
public:
	int start(float timeBetweenPulses, const std::string& pulseFileName, const std::vector<int>& outputInstances);
	void stop();

protected:
	static int processSamplesStatic(jack_nframes_t nframes, void* arg);
	int processSamples(jack_nframes_t nframes);

	static void onDoMeasureStatic(uv_timer_t* timer);
	void onDoMeasure();

private:
	PulseData pulseData;
	ServerControl serverControl;
	std::vector<PulseGenerator> pulseGenerators;
	jack_client_t* client;
	std::vector<jack_port_t*> inputPorts;
	std::vector<jack_port_t*> outputPorts;

	uv_timer_t doDelayMeasureTimer;
	std::atomic_bool triggerPulse;
	int samplesBetweenPulses;
	float sampleRate;
};

#endif
