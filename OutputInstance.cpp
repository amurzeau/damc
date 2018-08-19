#include "OutputInstance.h"
#include "ControlClient.h"
#include "ControlServer.h"
#include "gui/MessageInterface.h"
#include <algorithm>
#include <arpa/inet.h>
#include <fcntl.h>

OutputInstance::OutputInstance() : outputType(Local), client(nullptr), numChannel(0), updateLevelTimer(nullptr) {}

OutputInstance::~OutputInstance() {
	if(updateLevelTimer) {
		uv_timer_stop(updateLevelTimer);
		uv_close((uv_handle_t*) updateLevelTimer, &onCloseTimer);
	}
}

int OutputInstance::init(ControlServer* controlServer,
                         OutputType outputType,
                         int index,
                         size_t numChannel,
                         size_t numEq,
                         const char* ip,
                         int port) {
	jack_status_t status;
	this->controlServer = controlServer;
	this->outputType = outputType;
	this->outputInstance = index;

	sprintf(clientName, "waveSendUDP-%s-%d", outputType == Local ? "local" : "remote", index);

	printf("Opening jack client %s\n", clientName);
	client = jack_client_open(clientName, JackNullOption, &status);
	if(client == NULL) {
		printf("Failed to open jack: %d.\n", status);
		return -3;
	}

	this->numChannel = numChannel;
	this->sampleRate = jack_get_sample_rate(client);
	filters.init(this->sampleRate, numChannel, numEq);

	levelsDb.resize(numChannel);
	peaksPerChannel.resize(numChannel);
	inputPorts.resize(numChannel);
	outputPorts.resize(numChannel);
	for(size_t i = 0; i < numChannel; i++) {
		char name[64];

		sprintf(name, "input_%s_%zd", outputType == Local ? "local" : "remote", i + 1);

		inputPorts[i] = jack_port_register(client, name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
		if(inputPorts[i] == 0) {
			printf("cannot register input port \"%s\"!\n", name);
			jack_client_close(client);
			client = nullptr;
			return -4;
		}

		sprintf(name, "output_%s_%zd", outputType == Local ? "local" : "remote", i + 1);
		outputPorts[i] = jack_port_register(client, name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
		if(outputPorts[i] == 0) {
			printf("cannot register output port \"%s\"!\n", name);
			jack_client_close(client);
			client = nullptr;
			return -4;
		}
	}

	pthread_mutex_init(&filtersMutex, NULL);
	pthread_mutex_init(&peakMutex, NULL);

	if(outputType == Remote) {
		remoteUdpOutput.init(index, this->sampleRate, ip, port);
	}

	if(updateLevelTimer) {
		uv_timer_stop(updateLevelTimer);
		uv_close((uv_handle_t*) updateLevelTimer, &onCloseTimer);
	}
	updateLevelTimer = new uv_timer_t;
	uv_timer_init(uv_default_loop(), updateLevelTimer);
	updateLevelTimer->data = this;
	uv_timer_start(updateLevelTimer, &onTimeoutTimerStatic, 33, 33);  // 30fps

	jack_set_process_callback(client, &processSamplesStatic, this);

	int ret = jack_activate(client);
	if(ret) {
		printf("cannot activate client: %d\n", ret);
	}

	printf("Processing interface %d...\n", index);

	return 0;
}

int OutputInstance::messageProcessor(const void* data, size_t) {
	const control_message_t* message = (control_message_t*) data;

	printf("Received message %d\n", message->opcode);
	switch(message->opcode) {
		case control_message_t::op_set_mute:
			printf("Setting mute: %d\n", message->data.set_mute.mute);
			pthread_mutex_lock(&filtersMutex);
			filters.setMute(message->data.set_mute.mute);
			pthread_mutex_unlock(&filtersMutex);
			break;
		case control_message_t::op_set_volume:
			printf("Setting volume to %.1f dBFS\n", message->data.set_volume.volume);
			pthread_mutex_lock(&filtersMutex);
			filters.setVolumeDb(message->data.set_volume.volume);
			pthread_mutex_unlock(&filtersMutex);
			break;
		case control_message_t::op_set_eq:
			printf("Set EQ %d with f0 = %.3f, Q = %.3f, gain = %.3f\n",
			       message->data.set_eq.index,
			       message->data.set_eq.f0,
			       message->data.set_eq.Q,
			       message->data.set_eq.gain);
			pthread_mutex_lock(&filtersMutex);
			filters.setEqualizer(message->data.set_eq.index,
			                     (EqFilter::FilterType) message->data.set_eq.type,
			                     message->data.set_eq.f0,
			                     message->data.set_eq.gain,
			                     message->data.set_eq.Q);
			pthread_mutex_unlock(&filtersMutex);
			break;
		case control_message_t::op_set_delay:
			printf("Set Delay %.6fs\n", message->data.set_delay.delay);
			pthread_mutex_lock(&filtersMutex);
			filters.setDelay(message->data.set_delay.delay);
			pthread_mutex_unlock(&filtersMutex);
			break;
	}

	notification_message_t messageBroadcast;
	messageBroadcast.outputInstance = outputInstance;
	messageBroadcast.opcode = notification_message_t::op_control;
	messageBroadcast.data.control = *message;

	controlServer->sendMessage(&messageBroadcast, sizeof(messageBroadcast));

	return 0;
}

void OutputInstance::sendFilterParameters(ControlClient* client) {
	notification_message_t messageBroadcast;
	messageBroadcast.outputInstance = outputInstance;
	messageBroadcast.opcode = notification_message_t::op_control;
	messageBroadcast.data.control.outputInstance = this->outputInstance;

	messageBroadcast.data.control.opcode = control_message_t::op_set_mute;
	messageBroadcast.data.control.data.set_mute.mute = filters.isMute();
	client->sendMessage(&messageBroadcast, sizeof(messageBroadcast));

	messageBroadcast.data.control.opcode = control_message_t::op_set_volume;
	messageBroadcast.data.control.data.set_volume.volume = filters.getVolumeDb();
	client->sendMessage(&messageBroadcast, sizeof(messageBroadcast));

	messageBroadcast.data.control.opcode = control_message_t::op_set_eq;
	for(size_t i = 0; i < filters.getEqualizerNumber(); i++) {
		EqFilter::FilterType type;
		double f0, gain, Q;

		filters.getEqualizer(i, type, f0, gain, Q);
		messageBroadcast.data.control.data.set_eq.index = i;
		messageBroadcast.data.control.data.set_eq.type = (int) type;
		messageBroadcast.data.control.data.set_eq.f0 = f0;
		messageBroadcast.data.control.data.set_eq.gain = gain;
		messageBroadcast.data.control.data.set_eq.Q = Q;
		client->sendMessage(&messageBroadcast, sizeof(messageBroadcast));
	}

	messageBroadcast.data.control.opcode = control_message_t::op_set_delay;
	messageBroadcast.data.control.data.set_delay.delay = filters.getDelay();
	client->sendMessage(&messageBroadcast, sizeof(messageBroadcast));
}

int OutputInstance::processSamplesStatic(jack_nframes_t nframes, void* arg) {
	OutputInstance* thisInstance = (OutputInstance*) arg;

	if(thisInstance->outputType == Local)
		return thisInstance->processSamplesLocal(nframes);
	else
		return thisInstance->processSamplesRemote(nframes);
}

int OutputInstance::processSamplesLocal(jack_nframes_t nframes) {
	double peaks[numChannel];

	for(size_t i = 0; i < numChannel; i++) {
		jack_default_audio_sample_t* in;
		jack_default_audio_sample_t* out;
		double filteringBuffer[nframes];

		in = (jack_default_audio_sample_t*) jack_port_get_buffer(inputPorts[i], nframes);
		out = (jack_default_audio_sample_t*) jack_port_get_buffer(outputPorts[i], nframes);

		std::copy(in, in + nframes, filteringBuffer);

		pthread_mutex_lock(&filtersMutex);
		filters.processSamples(filteringBuffer, nframes, i);
		pthread_mutex_unlock(&filtersMutex);

		std::copy(filteringBuffer, filteringBuffer + nframes, out);

		peaks[i] = std::abs(*std::max_element(filteringBuffer, filteringBuffer + nframes));
	}

	pthread_mutex_lock(&peakMutex);
	this->samplesInPeaks += nframes;
	for(size_t i = 0; i < numChannel; i++) {
		this->peaksPerChannel[i] = std::max(peaks[i], this->peaksPerChannel[i]);
	}
	pthread_mutex_unlock(&peakMutex);

	return 0;
}

int OutputInstance::processSamplesRemote(jack_nframes_t nframes) {
	int16_t outBuffers[2][nframes];
	double peaks[numChannel];

	for(double& peak : peaks) {
		peak = 0;
	}

	for(size_t i = 0; i < 2; i++) {
		jack_default_audio_sample_t* in;
		double filteringBuffer[nframes];

		in = (jack_default_audio_sample_t*) jack_port_get_buffer(inputPorts[i % numChannel], nframes);

		std::copy(in, in + nframes, filteringBuffer);

		pthread_mutex_lock(&filtersMutex);
		filters.processSamples(filteringBuffer, nframes, i);
		pthread_mutex_unlock(&filtersMutex);

		std::transform(filteringBuffer, filteringBuffer + nframes, outBuffers[i], [](double sample) {
			return int16_t(sample * 32768);
		});

		if(i < numChannel) {
			peaks[i] = std::abs(*std::max_element(filteringBuffer, filteringBuffer + nframes));
		}
	}

	for(size_t i = 0; i < 2; i++) {
		jack_default_audio_sample_t* out;

		out = (jack_default_audio_sample_t*) jack_port_get_buffer(outputPorts[i], nframes);

		std::transform(outBuffers[i], outBuffers[i] + nframes, out, [](jack_default_audio_sample_t sample) {
			return jack_default_audio_sample_t(sample / 32768.0);
		});
	}

	pthread_mutex_lock(&peakMutex);
	this->samplesInPeaks += nframes;
	for(size_t i = 0; i < numChannel; i++) {
		this->peaksPerChannel[i] = std::max(peaks[i], this->peaksPerChannel[i]);
	}
	pthread_mutex_unlock(&peakMutex);

	remoteUdpOutput.sendAudio(outBuffers[0], outBuffers[1], nframes);
	return 0;
}

void OutputInstance::onTimeoutTimerStatic(uv_timer_t* handle) {
	OutputInstance* thisInstance = (OutputInstance*) handle->data;
	thisInstance->onTimeoutTimer();
}

void OutputInstance::onTimeoutTimer() {
	int samples;
	std::vector<double> peaks;

	pthread_mutex_lock(&peakMutex);
	samples = this->samplesInPeaks;
	peaks = this->peaksPerChannel;
	this->samplesInPeaks = 0;
	for(double& peak : this->peaksPerChannel) {
		peak = 0;
	}
	pthread_mutex_unlock(&peakMutex);

	double deltaT = (double) samples / this->sampleRate;

	for(size_t channel = 0; channel < peaks.size(); channel++) {
		double peakDb = peaks[channel] != 0 ? 20.0 * log10(peaks[channel]) : -INFINITY;

		double decayAmount = 11.76470588235294 * deltaT;  // -20dB / 1.7s
		double levelDb = std::max(levelsDb[channel] - decayAmount, peakDb);
		levelsDb[channel] = levelDb > -192 ? levelDb : -INFINITY;
	}

	notification_message_t message;
	message.outputInstance = outputInstance;
	message.opcode = notification_message_t::op_level;
	for(size_t i = 0; i < sizeof(message.data.level.level) / sizeof(message.data.level.level[0]); i++) {
		if(i < levelsDb.size())
			message.data.level.level[i] = levelsDb[i];
		else
			message.data.level.level[i] = -INFINITY;
	}
	controlServer->sendMessage(&message, sizeof(message));
}

void OutputInstance::onCloseTimer(uv_handle_t* handle) {
	delete(uv_timer_t*) handle;
}
