#include "BinauralSpacializer.h"
#include <stdio.h>
#include <string.h>
#include <string>

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

int openJackPort(jack_client_t* client, jack_port_t** ports, size_t number, JackPortFlags type) {
	char name[64];

	printf("Registering %d %s ports\n", number, (type & JackPortIsInput) ? "input" : "output");

	for(size_t i = 0; i < number; i++) {
		if(type & JackPortIsInput)
			sprintf(name, "input_%zd", i + 1);
		else
			sprintf(name, "output_%zd", i + 1);

		ports[i] = jack_port_register(client, name, JACK_DEFAULT_AUDIO_TYPE, type, 0);
		if(ports[i] == 0) {
			printf("cannot register port \"%s\"!\n", name);
			return -1;
		}
	}

	return 0;
}

BinauralSpacializer::BinauralSpacializer() : mysofaHandle(nullptr, &mysofa_close) {}

int BinauralSpacializer::start(unsigned int numChannels, const std::string& pulseFileName) {
	jack_status_t status;
	std::string clientName = "binauralSpacializer";

	printf("Opening jack client %s\n", clientName.c_str());
	client = jack_client_open(clientName.c_str(), JackNullOption, &status);
	if(client == nullptr) {
		printf("Failed to open jack: %d.\n", status);
		return -3;
	}

	printf("Opened jack client\n");

	sampleRate = jack_get_sample_rate(client);

	int err;
	int filterLength;
	mysofaHandle.reset(mysofa_open(pulseFileName.c_str(), sampleRate, &filterLength, &err));
	if(!mysofaHandle) {
		printf("Failed to open HRTF SOFA file %s, error %d\n", pulseFileName.c_str(), err);
		return -4;
	}

	printf("SOFA Attributes: I: %d, C: %d, R: %d, E: %d, N: %d, M: %d\n",
	       mysofaHandle->hrtf->I,
	       mysofaHandle->hrtf->C,
	       mysofaHandle->hrtf->R,
	       mysofaHandle->hrtf->E,
	       mysofaHandle->hrtf->N,
	       mysofaHandle->hrtf->M);
	for(struct MYSOFA_ATTRIBUTE* attribute = mysofaHandle->hrtf->attributes; attribute; attribute = attribute->next) {
		printf(" %s: %s\n", attribute->name, attribute->value);
	}

	fillTransforms(mysofaHandle.get(), filterLength, numChannels);

	inputPorts.resize(numChannels);
	if(openJackPort(client, &inputPorts[0], inputPorts.size(), JackPortIsInput) < 0)
		return -1;

	if(openJackPort(client, &outputPorts[0], outputPorts.size(), JackPortIsOutput) < 0)
		return -1;

	jack_set_process_callback(client, &processSamplesStatic, this);

	int ret = jack_activate(client);
	if(ret) {
		printf("cannot activate client: %d\n", ret);
	}

	return 0;
}

void BinauralSpacializer::stop() {
	jack_deactivate(client);
	jack_client_close(client);
	mysofaHandle.reset();
}

int BinauralSpacializer::processSamplesStatic(jack_nframes_t nframes, void* arg) {
	BinauralSpacializer* thisInstance = (BinauralSpacializer*) arg;
	return thisInstance->processSamples(nframes);
}

int BinauralSpacializer::processSamples(jack_nframes_t nframes) {
	jack_default_audio_sample_t* out[outputPorts.size()];
	float buffer[nframes];
	size_t inputNumber = std::min(inputPorts.size(), transforms.size());

	for(size_t i = 0; i < outputPorts.size(); i++) {
		out[i] = (jack_default_audio_sample_t*) jack_port_get_buffer(outputPorts[i], nframes);
		memset(out[i], 0, nframes * sizeof(out[i][0]));
	}

	for(size_t i = 0; i < inputNumber; i++) {
		jack_default_audio_sample_t* in = (jack_default_audio_sample_t*) jack_port_get_buffer(inputPorts[i], nframes);

		for(size_t ear = 0; ear < outputPorts.size(); ear++) {
			transforms[i].ears[ear].hrtf.processSamples(buffer, in, nframes);
			transforms[i].ears[ear].delay.processSamples(buffer, buffer, nframes);

			// Mix the result in output
			for(size_t j = 0; j < nframes; j++)
				out[ear][j] += buffer[j];
		}
	}

	return 0;
}

void BinauralSpacializer::fillTransforms(struct MYSOFA_EASY* mysofaHandle, int filterLength, size_t numChannels) {
	struct Position {
		float x, y, z;
	};

	std::vector<Position> channelsPosition(numChannels);
	std::vector<float> left, right;
	float delayLeft, delayRight;

	static const std::vector<std::vector<std::pair<float, float>>> speakerConfigurations = {
	    {{0, 0}},                                                                     // 1.0
	    {{30, 0}, {-30, 0}},                                                          // 2.0
	    {{30, 0}, {-30, 0}, {0, 0}},                                                  // 2.1 or 3.0
	    {{30, 0}, {-30, 0}, {90, 0}, {-90, 0}},                                       // 4.0
	    {{30, 0}, {-30, 0}, {0, 0}, {90, 0}, {-90, 0}},                               // 5.0 or 4.1
	    {{30, 0}, {-30, 0}, {0, 0}, {0, 0}, {110, 0}, {-110, 0}},                     // 5.1
	    {{30, 0}, {-30, 0}, {0, 0}, {0, 0}, {180, 0}, {90, 0}, {-90, 0}},             // 6.1
	    {{30, 0}, {-30, 0}, {0, 0}, {0, 0}, {140, 0}, {-140, 0}, {90, 0}, {-90, 0}},  // 7.1
	    //{{51, 24}, {-51, 24}, {0, 0}, {0, 0}, {180, 55}, {0, -55}, {129, -24}, {-129, -24}},  // 3D7.1
	};

	const std::vector<std::pair<float, float>>& speakerConfiguration = speakerConfigurations[numChannels - 1];

	if(speakerConfiguration.size() != numChannels) {
		printf("Bad channel configuration number %d for numChannels %d, should be equal !\n",
		       (int) speakerConfiguration.size(),
		       (int) numChannels);
	}

	printf("Channel configuration: %d channels, filterLength: %d\n", (int) speakerConfiguration.size(), filterLength);
	for(std::pair<float, float> angles : speakerConfiguration)
		printf(" - azimut: %.3f deg, elevation: %.3f def\n", angles.first, angles.second);

	left.resize(filterLength);
	right.resize(filterLength);

	transforms.resize(speakerConfiguration.size());
	for(size_t i = 0; i < speakerConfiguration.size(); i++) {
		float pos[3] = {speakerConfiguration[i].first, speakerConfiguration[i].second, 1.95};

		// convert to cartesian coords
		mysofa_s2c(pos);

		mysofa_getfilter_float(
		    mysofaHandle, pos[0], pos[1], pos[2], left.data(), right.data(), &delayLeft, &delayRight);

		transforms[i].ears[0].hrtf.setConvolver(left.data(), left.size());
		transforms[i].ears[0].delay.init();
		transforms[i].ears[0].delay.setParameters(delayLeft * sampleRate);

		transforms[i].ears[1].hrtf.setConvolver(right.data(), right.size());
		transforms[i].ears[1].delay.init();
		transforms[i].ears[1].delay.setParameters(delayRight * sampleRate);
	}
}
