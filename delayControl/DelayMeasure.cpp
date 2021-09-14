#include "DelayMeasure.h"
#include "WavLoader.h"
#include <stdio.h>
#include <string>

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

int openJackPort(jack_client_t* client, jack_port_t** ports, size_t number, JackPortFlags type) {
	char name[64];

	printf("Registering %d %s ports\n", (int) number, (type & JackPortIsInput) ? "input" : "output");

	for(size_t i = 0; i < number; i++) {
		if(type & JackPortIsInput)
			sprintf(name, "input_%d", (int) (i + 1));
		else if(i < number - 3)
			sprintf(name, "output_%d", (int) (i + 1));
		else
			sprintf(name, "debug_sync_%d", (int) (i + 1));

		ports[i] = jack_port_register(client, name, JACK_DEFAULT_AUDIO_TYPE, type, 0);
		if(ports[i] == 0) {
			printf("cannot register port \"%s\"!\n", name);
			return -1;
		}
	}

	return 0;
}

int DelayMeasure::start(float timeBetweenPulses,
                        const std::string& pulseFileName,
                        int thresholdRatio,
                        const std::vector<int>& outputInstances) {
	jack_status_t status;
	std::string clientName = "delayControl";

	if(outputInstances.size() < 1) {
		printf("No output instance declared !\n");
		return -4;
	}

	serverControl.init(outputInstances[0]);

	printf("Opening jack client %s\n", clientName.c_str());
	client = jack_client_open(clientName.c_str(), JackNullOption, &status);
	if(client == nullptr) {
		printf("Failed to open jack: %d.\n", status);
		return -3;
	}

	printf("Opened jack client\n");

	sampleRate = jack_get_sample_rate(client);
	this->samplesBetweenPulses = timeBetweenPulses * sampleRate;

	pulseData.open(pulseFileName, jack_get_buffer_size(client), sampleRate, thresholdRatio);

	inputPorts.resize(1);
	if(openJackPort(client, &inputPorts[0], inputPorts.size(), JackPortIsInput) < 0)
		return -1;
	outputPorts.resize(outputInstances.size() + 3);
	if(openJackPort(client, &outputPorts[0], outputPorts.size(), JackPortIsOutput) < 0)
		return -1;

	for(size_t i = 0; i < outputInstances.size(); i++) {
		pulseGenerators.emplace_back(
		    pulseData.getData(), i * samplesBetweenPulses, &serverControl, outputInstances[i], sampleRate);
		serverControl.adjustDelay(outputInstances[i], 0);
	}

	jack_set_process_callback(client, &processSamplesStatic, this);

	uv_timer_init(uv_default_loop(), &doDelayMeasureTimer);
	doDelayMeasureTimer.data = this;

	int ret = jack_activate(client);
	if(ret) {
		printf("cannot activate client: %d\n", ret);
	}

	uv_timer_start(&doDelayMeasureTimer, &onDoMeasureStatic, 0, 1000);

	return 0;
}

void DelayMeasure::stop() {
	uv_timer_stop(&doDelayMeasureTimer);
	uv_close((uv_handle_t*) &doDelayMeasureTimer, NULL);
	jack_deactivate(client);
	jack_client_close(client);
	serverControl.stop();
}

int DelayMeasure::processSamplesStatic(jack_nframes_t nframes, void* arg) {
	DelayMeasure* thisInstance = (DelayMeasure*) arg;
	return thisInstance->processSamples(nframes);
}

int DelayMeasure::processSamples(jack_nframes_t nframes) {
	jack_default_audio_sample_t* in;

	if(triggerPulse) {
		triggerPulse = false;
		for(size_t i = 0; i < pulseGenerators.size(); i++) {
			pulseGenerators[i].startNewPulse();
		}
	}

	for(size_t i = 0; i < pulseGenerators.size(); i++) {
		jack_default_audio_sample_t* out = (jack_default_audio_sample_t*) jack_port_get_buffer(outputPorts[i], nframes);
		pulseGenerators[i].processSamples(out, nframes);
	}

	in = (jack_default_audio_sample_t*) jack_port_get_buffer(inputPorts[0], nframes);
	jack_default_audio_sample_t* outs[3] = {
	    (jack_default_audio_sample_t*) jack_port_get_buffer(outputPorts[outputPorts.size() - 3], nframes),
	    (jack_default_audio_sample_t*) jack_port_get_buffer(outputPorts[outputPorts.size() - 2], nframes),
	    (jack_default_audio_sample_t*) jack_port_get_buffer(outputPorts[outputPorts.size() - 1], nframes)};

	pulseData.doCrossCorrelation(in, outs[0], outs[1], outs[2], nframes);

	return 0;
}

void DelayMeasure::onDoMeasureStatic(uv_timer_t* timer) {
	DelayMeasure* thisInstance = (DelayMeasure*) timer->data;
	return thisInstance->onDoMeasure();
}
void DelayMeasure::onDoMeasure() {
	std::vector<uint64_t> pulses = pulseData.retrieveDetectedPulses();
	for(size_t i = 1; i < pulses.size(); i++) {
		int64_t diff = pulses[i] - pulses[0];
		if(i < pulseGenerators.size())
			pulseGenerators[i].computeDelayControl(diff - samplesBetweenPulses * i, pulses[0]);
	}
	serverControl.updateDelays();
	printf("New pulse with %.0f ms delay\n", samplesBetweenPulses / sampleRate * 1000);
	triggerPulse = true;
}
