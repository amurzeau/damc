#include "LoopbackOutputInstance.h"
#include "ControlClient.h"
#include "ControlServer.h"
#include <algorithm>

const char* LoopbackOutputInstance::getName() {
	return "loopback";
}

int LoopbackOutputInstance::configureJackPorts(jack_client_t* client, size_t numChannel) {
	inputPorts.resize(numChannel);
	outputPorts.resize(numChannel);
	for(size_t i = 0; i < numChannel; i++) {
		char name[64];

		sprintf(name, "input_%zd", i + 1);

		inputPorts[i] = jack_port_register(client, name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
		if(inputPorts[i] == 0) {
			printf("cannot register input port \"%s\"!\n", name);
			jack_client_close(client);
			client = nullptr;
			return -4;
		}

		sprintf(name, "output_%zd", i + 1);
		outputPorts[i] = jack_port_register(client, name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
		if(outputPorts[i] == 0) {
			printf("cannot register output port \"%s\"!\n", name);
			jack_client_close(client);
			client = nullptr;
			return -4;
		}
	}
	return 0;
}

int LoopbackOutputInstance::processSamples(size_t numChannel, jack_nframes_t nframes) {
	double peaks[32];

	for(size_t i = 0; i < numChannel; i++) {
		jack_default_audio_sample_t* in;
		jack_default_audio_sample_t* out;
		double filteringBuffer[4096];

		in = (jack_default_audio_sample_t*) jack_port_get_buffer(inputPorts[i], nframes);
		out = (jack_default_audio_sample_t*) jack_port_get_buffer(outputPorts[i], nframes);

		std::copy(in, in + nframes, (double*) filteringBuffer);

		filterSamples(filteringBuffer, nframes, i);

		std::copy(filteringBuffer, filteringBuffer + nframes, out);

		peaks[i] = std::abs(*std::max_element((double*) filteringBuffer, filteringBuffer + nframes));
	}

	return 0;
}
