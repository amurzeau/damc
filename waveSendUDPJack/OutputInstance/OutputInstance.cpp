#include "OutputInstance.h"
#include "../ControlInterface.h"
#include "../ControlServer.h"
#include <algorithm>
#include <jack/metadata.h>
#include <jack/uuid.h>

OutputInstance::OutputInstance(IAudioEndpoint* endpoint)
    : endpoint(endpoint),
      enabled(true),
      client(nullptr),
      numChannel(0),
      controlSettings(nlohmann::json::object()),
      updateLevelTimer(nullptr) {}

OutputInstance::~OutputInstance() {
	if(updateLevelTimer) {
		uv_timer_stop(updateLevelTimer);
		uv_close((uv_handle_t*) updateLevelTimer, &onCloseTimer);
	}

	if(client) {
		jack_deactivate(client);
		jack_client_close(client);
		client = nullptr;
	}

	uv_mutex_destroy(&peakMutex);
	uv_mutex_destroy(&filtersMutex);
}

int OutputInstance::init(ControlInterface* controlInterface,
                         ControlServer* controlServer,
                         int type,
                         int index,
                         size_t numChannel,
                         const nlohmann::json& json) {
	char buffer[128];

	this->controlInterface = controlInterface;
	this->controlServer = controlServer;
	this->outputInstance = index;
	this->type = type;

	if(numChannel > 32) {
		printf("Too many channels %d, max supported %d\n", numChannel, 32);
		return -2;
	}

	this->numChannel = numChannel;

	levelsDb.resize(numChannel, -192);
	samplesInPeaks = 0;
	peaksPerChannel.resize(numChannel, 0);

	uv_mutex_init(&filtersMutex);
	uv_mutex_init(&peakMutex);

	sprintf(buffer, "waveSendUDP-%s-%d", endpoint->getName(), index);
	clientName = buffer;
	clientDisplayName = clientName;

	filters.init(numChannel);

	setParameters(json);

	if(enabled)
		start();

	return 0;
}

int OutputInstance::start() {
	jack_status_t status;

	if(client)
		return 0;

	printf("Opening jack client %s\n", clientName.c_str());
	client = jack_client_open(clientName.c_str(), JackNullOption, &status);
	if(client == NULL) {
		printf("Failed to open jack: %d.\n", status);
		return -3;
	}

	printf("Opened jack client\n");

	this->sampleRate = jack_get_sample_rate(client);
	filters.reset(this->sampleRate);

	int additionnalPortFlags = 0;
	if(type != Loopback)
		additionnalPortFlags |= JackPortIsTerminal;

	inputPorts.resize(numChannel);
	outputPorts.resize(numChannel);
	for(size_t i = 0; i < numChannel; i++) {
		char name[64];

		sprintf(name, "input_%zd", i + 1);

		inputPorts[i] =
		    jack_port_register(client, name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput | additionnalPortFlags, 0);
		if(inputPorts[i] == 0) {
			printf("cannot register input port \"%s\"!\n", name);
			jack_client_close(client);
			client = nullptr;
			return -4;
		}

		sprintf(name, "output_%zd", i + 1);
		outputPorts[i] =
		    jack_port_register(client, name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput | additionnalPortFlags, 0);
		if(outputPorts[i] == 0) {
			printf("cannot register output port \"%s\"!\n", name);
			jack_client_close(client);
			client = nullptr;
			return -4;
		}
	}

	if(type != DeviceInput && type != RemoteInput) {
		inputPorts.push_back(jack_port_register(
		    client, "side_channel", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput | additionnalPortFlags, 0));
		if(inputPorts.back() == 0) {
			printf("cannot register input port \"%s\"!\n", "side_channel");
			jack_client_close(client);
			client = nullptr;
			return -4;
		}
	}

	endpoint->start(outputInstance, numChannel, sampleRate, jack_get_buffer_size(client));

	if(updateLevelTimer) {
		uv_timer_stop(updateLevelTimer);
		uv_close((uv_handle_t*) updateLevelTimer, &onCloseTimer);
	}
	updateLevelTimer = new uv_timer_t;
	uv_timer_init(uv_default_loop(), updateLevelTimer);
	updateLevelTimer->data = this;
	uv_timer_start(updateLevelTimer, &onTimeoutTimerStatic, 66, 66);  // 15fps

	jack_set_process_callback(client, &processSamplesStatic, this);

	const char* pszClientUuid = ::jack_get_uuid_for_client_name(client, clientName.c_str());
	if(pszClientUuid) {
		::jack_uuid_parse(pszClientUuid, &clientUuid);
		if(clientDisplayName != clientName) {
			::jack_set_property(client, clientUuid, JACK_METADATA_PRETTY_NAME, clientDisplayName.c_str(), NULL);
		}
		jack_free((void*) pszClientUuid);
	} else {
		clientUuid = 0;
	}
	jack_set_property_change_callback(client, &OutputInstance::onJackPropertyChangeCallback, this);

	int ret = jack_activate(client);
	if(ret) {
		printf("cannot activate client: %d\n", ret);
	}

	printf("Processing interface %d...\n", outputInstance);

	return 0;
}

void OutputInstance::stop() {
	printf("Stopping jack client %s\n", clientName.c_str());

	if(updateLevelTimer) {
		uv_timer_stop(updateLevelTimer);
		uv_close((uv_handle_t*) updateLevelTimer, &onCloseTimer);
		updateLevelTimer = nullptr;
	}

	if(client) {
		jack_deactivate(client);
		jack_client_close(client);
		client = nullptr;

		endpoint->stop();
	}
}

void OutputInstance::onJackPropertyChangeCallback(jack_uuid_t subject,
                                                  const char* key,
                                                  jack_property_change_t change,
                                                  void* arg) {
	OutputInstance* thisInstance = (OutputInstance*) arg;
	char* pszValue = NULL;
	char* pszType = NULL;

	if(subject == thisInstance->clientUuid && strcmp(key, JACK_METADATA_PRETTY_NAME) == 0) {
		switch(change) {
			case PropertyCreated:
			case PropertyChanged:
				::jack_get_property(thisInstance->clientUuid, JACK_METADATA_PRETTY_NAME, &pszValue, &pszType);
				if(pszValue) {
					thisInstance->clientDisplayName = pszValue;
					::jack_free(pszValue);
				}
				if(pszType)
					::jack_free(pszType);
				break;
			case PropertyDeleted:
				thisInstance->clientDisplayName = thisInstance->clientName;
				break;
		}

		nlohmann::json json = {{"instance", thisInstance->outputInstance},
		                       {"displayName", thisInstance->clientDisplayName}};
		std::string jsonStr = json.dump();
		thisInstance->controlServer->sendMessage(jsonStr.c_str(), jsonStr.size());
		thisInstance->controlInterface->saveConfig();
	}
}

void OutputInstance::setParameters(const nlohmann::json& json) {
	bool newEnabledState = enabled;

	uv_mutex_lock(&filtersMutex);
	try {
		clientName = json.value("name", clientName);
		clientDisplayName = json.value("displayName", clientDisplayName);
		newEnabledState = json.value("enabled", enabled);
		if(json.find("controlSettings") != json.end())
			controlSettings = json["controlSettings"];
		filters.setParameters(json);
		endpoint->setParameters(json);
	} catch(const nlohmann::json::exception& e) {
		printf("Exception while processing message: %s\n", e.what());
	} catch(...) {
		printf("Exception while processing message\n");
	}
	uv_mutex_unlock(&filtersMutex);

	printf("Old state: %d, new state: %d\n", enabled, newEnabledState);
	if(newEnabledState != enabled && newEnabledState) {
		enabled = newEnabledState;
		start();
	} else if(newEnabledState != enabled && !newEnabledState) {
		enabled = newEnabledState;
		stop();
	}
}

nlohmann::json OutputInstance::getParameters() {
	nlohmann::json json = filters.getParameters();
	nlohmann::json subClassJson = endpoint->getParameters();
	json["enabled"] = enabled;
	json["instance"] = outputInstance;
	json["type"] = type;
	json["numChannels"] = numChannel;
	json["name"] = clientName;
	json["displayName"] = clientDisplayName;
	json["controlSettings"] = controlSettings;

	for(const auto& j : subClassJson.items()) {
		json[j.key()] = j.value();
	}

	return json;
}

int OutputInstance::processSamplesStatic(jack_nframes_t nframes, void* arg) {
	OutputInstance* thisInstance = (OutputInstance*) arg;

	if(thisInstance->type != DeviceInput && thisInstance->type != RemoteInput)
		return thisInstance->processSamples(nframes);
	else
		return thisInstance->processInputSamples(nframes);
}

int OutputInstance::processInputSamples(jack_nframes_t nframes) {
	float* buffers[32];
	float peaks[32];

	if(numChannel > sizeof(buffers) / sizeof(buffers[0])) {
		printf("Too many channels, buffer too small !!!\n");
	}

	for(size_t i = 0; i < numChannel; i++) {
		buffers[i] = (jack_default_audio_sample_t*) jack_port_get_buffer(outputPorts[i], nframes);
	}

	endpoint->postProcessSamples(buffers, numChannel, nframes);

	uv_mutex_lock(&filtersMutex);
	filters.processSamples(peaks, buffers, const_cast<const float**>(buffers), numChannel, nframes);
	uv_mutex_unlock(&filtersMutex);

	uv_mutex_lock(&peakMutex);
	this->samplesInPeaks += nframes;
	for(size_t i = 0; i < numChannel; i++) {
		this->peaksPerChannel[i] = std::max(peaks[i], this->peaksPerChannel[i]);
	}
	uv_mutex_unlock(&peakMutex);

	return 0;
}

int OutputInstance::processSamples(jack_nframes_t nframes) {
	float* outputs[32];
	const float* inputs[32];
	float peaks[32];

	if(numChannel > sizeof(outputs) / sizeof(outputs[0])) {
		printf("Too many channels, buffer too small !!!\n");
	}

	for(size_t i = 0; i < numChannel; i++) {
		inputs[i] = (jack_default_audio_sample_t*) jack_port_get_buffer(inputPorts[i], nframes);
		outputs[i] = (jack_default_audio_sample_t*) jack_port_get_buffer(outputPorts[i], nframes);
	}

	uv_mutex_lock(&filtersMutex);
	filters.processSamples(peaks, outputs, inputs, numChannel, nframes);
	uv_mutex_unlock(&filtersMutex);

	// add side channel on channel 0
	if(numChannel < inputPorts.size()) {
		jack_default_audio_sample_t* sideChannel =
		    (jack_default_audio_sample_t*) jack_port_get_buffer(inputPorts[numChannel], nframes);
		for(jack_nframes_t i = 0; i < nframes; i++) {
			outputs[0][i] += filters.processSideChannelSample(sideChannel[i]);
		}
	}

	uv_mutex_lock(&peakMutex);
	this->samplesInPeaks += nframes;
	for(size_t i = 0; i < numChannel; i++) {
		this->peaksPerChannel[i] = std::max(peaks[i], this->peaksPerChannel[i]);
	}
	uv_mutex_unlock(&peakMutex);

	endpoint->postProcessSamples(outputs, numChannel, nframes);

	return 0;
}

void OutputInstance::onTimeoutTimerStatic(uv_timer_t* handle) {
	OutputInstance* thisInstance = (OutputInstance*) handle->data;
	thisInstance->onTimeoutTimer();
}

void OutputInstance::onTimeoutTimer() {
	int samples;
	std::vector<float> peaks(this->peaksPerChannel.size(), 0);

	uv_mutex_lock(&peakMutex);
	samples = this->samplesInPeaks;
	peaks.swap(this->peaksPerChannel);
	this->samplesInPeaks = 0;
	uv_mutex_unlock(&peakMutex);

	if(this->sampleRate == 0)
		return;

	float deltaT = (float) samples / this->sampleRate;

	for(size_t channel = 0; channel < peaks.size(); channel++) {
		float peakDb = peaks[channel] != 0 ? 20.0 * log10(peaks[channel]) : -INFINITY;

		float decayAmount = 11.76470588235294 * deltaT;  // -20dB / 1.7s
		float levelDb = std::max(levelsDb[channel] - decayAmount, peakDb);
		levelsDb[channel] = levelDb > -192 ? levelDb : -192;
	}

	nlohmann::json json = {{"instance", outputInstance}, {"levels", levelsDb}};
	std::string jsonStr = json.dump();
	controlServer->sendMessage(jsonStr.c_str(), jsonStr.size());

	endpoint->onTimer();
}

void OutputInstance::onCloseTimer(uv_handle_t* handle) {
	delete(uv_timer_t*) handle;
}
