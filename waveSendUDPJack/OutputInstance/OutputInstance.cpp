#include "OutputInstance.h"
#include "../ControlInterface.h"
#include "../ControlServer.h"
#include <algorithm>
#include <jack/metadata.h>
#include <jack/uuid.h>

#include "DeviceInputInstance.h"
#include "DeviceOutputInstance.h"
#include "LoopbackOutputInstance.h"
#include "RemoteInputInstance.h"
#include "RemoteOutputInstance.h"
#ifdef _WIN32
#include "WasapiInstance.h"
#endif

OutputInstance::OutputInstance(OscContainer* parent, ControlInterface* controlInterface, int index)
    : OscContainer(parent, std::to_string(index)),
      outputInstance(index),
      controlInterface(controlInterface),
      client(nullptr),
      filters(this),

      oscEnable(this, "enable", true),
      oscType(this, "type", (int32_t) OutputInstance::None),
      oscName(this, "name"),
      oscDisplayName(this, "display_name"),
      oscNumChannel(this, "channels", 0),
      oscSampleRate(this, "sample_rate"),

      oscEnablePeakUpdate(this, "enable_peak", false),
      oscEnablePeakJsonUpdate(this, "enable_peak_json", true) {
	uv_mutex_init(&filtersMutex);
	uv_mutex_init(&peakMutex);

	oscPeakGlobalPath = getFullAddress() + "/meter";
	oscPeakPerChannelPath = getFullAddress() + "/meter_per_channel";

	oscType.addCheckCallback([this](int newValue) -> bool {
		if(endpoint) {
			printf("Endpoint type already set\n");
			return false;
		}

		switch(newValue) {
			case OutputInstance::Loopback:
			case OutputInstance::RemoteOutput:
			case OutputInstance::RemoteInput:
			case OutputInstance::DeviceOutput:
			case OutputInstance::DeviceInput:
#ifdef _WIN32
			case OutputInstance::WasapiDeviceOutput:
			case OutputInstance::WasapiDeviceInput:
#endif
				break;

			case OutputInstance::None:
				return false;
				break;

			default:
				printf("Bad type %d\n", newValue);
				return false;
		}

		return true;
	});

	oscType.setChangeCallback([this](int newValue) { updateType(newValue); });

	oscNumChannel.addCheckCallback([this](int32_t newValue) {
		if(client) {
			printf("Can't change channel number when output is enabled\n");
			return false;
		}

		if(newValue > 32) {
			printf("Too many channels: %d\n", newValue);
			return false;
		}

		if(newValue <= 0) {
			printf("Invalid value: %d\n", newValue);
			return false;
		}

		return true;
	});

	oscNumChannel.setChangeCallback([this](int32_t newValue) {
		printf("Changing channel number to %d\n", newValue);

		levelsDb.resize(newValue, -192);
		peaksPerChannel.resize(newValue, 0);
		samplesInPeaks = 0;

		filters.init(newValue);
	});

	readyChecker.addVariable(&oscEnable, true);
	readyChecker.addVariable(&oscType);
	readyChecker.addVariable(&oscNumChannel);

	readyChecker.setCallback([this]() {
		//
		printf("Output instance %d ready\n", this->outputInstance);
	});

	oscEnable.setChangeCallback([this](bool newValue) { updateEnabledState(newValue); });
}

OutputInstance::~OutputInstance() {
	if(client) {
		jack_deactivate(client);
		jack_client_close(client);
		client = nullptr;
	}

	uv_mutex_destroy(&peakMutex);
	uv_mutex_destroy(&filtersMutex);
}

int OutputInstance::start() {
	jack_status_t status;

	if(client)
		return 0;

	printf("Opening jack client %s\n", oscName.c_str());
	client = jack_client_open(oscName.c_str(), JackNullOption, &status);
	if(client == NULL) {
		printf("Failed to open jack: %d.\n", status);
		return -3;
	}

	printf("Opened jack client\n");

	jackSampleRate = jack_get_sample_rate(client);
	filters.reset(jackSampleRate);
	oscSampleRate.setDefault(jackSampleRate);

	int additionnalPortFlags = 0;
	if(oscType != Loopback)
		additionnalPortFlags |= JackPortIsTerminal;

	inputPorts.resize(oscNumChannel);
	outputPorts.resize(oscNumChannel);
	for(int32_t i = 0; i < oscNumChannel; i++) {
		char name[64];

		sprintf(name, "input_%d", (int) (i + 1));

		inputPorts[i] =
		    jack_port_register(client, name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput | additionnalPortFlags, 0);
		if(inputPorts[i] == 0) {
			printf("cannot register input port \"%s\"!\n", name);
			jack_client_close(client);
			client = nullptr;
			return -4;
		}

		sprintf(name, "output_%d", (int) (i + 1));
		outputPorts[i] =
		    jack_port_register(client, name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput | additionnalPortFlags, 0);
		if(outputPorts[i] == 0) {
			printf("cannot register output port \"%s\"!\n", name);
			jack_client_close(client);
			client = nullptr;
			return -4;
		}
	}

	if(oscType != DeviceInput && oscType != RemoteInput) {
		inputPorts.push_back(jack_port_register(
		    client, "side_channel", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput | additionnalPortFlags, 0));
		if(inputPorts.back() == 0) {
			printf("cannot register input port \"%s\"!\n", "side_channel");
			jack_client_close(client);
			client = nullptr;
			return -4;
		}
	}

	endpoint->start(outputInstance, oscNumChannel, jackSampleRate, jack_get_buffer_size(client));

	jack_set_process_callback(client, &processSamplesStatic, this);

	const char* pszClientUuid = ::jack_get_uuid_for_client_name(client, oscName.c_str());
	if(pszClientUuid) {
		::jack_uuid_parse(pszClientUuid, &clientUuid);
		if(!oscDisplayName.get().empty() && oscDisplayName != oscName) {
			::jack_set_property(client, clientUuid, JACK_METADATA_PRETTY_NAME, oscDisplayName.c_str(), NULL);
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
	printf("Stopping jack client %s\n", oscName.c_str());

	if(client) {
		jack_deactivate(client);
		jack_client_close(client);
		client = nullptr;

		endpoint->stop();
	}
}

bool OutputInstance::updateType(int newValue) {
	if(endpoint) {
		printf("Endpoint type already set\n");
		return false;
	}

	IAudioEndpoint* newEndpoint = nullptr;

	switch(newValue) {
		case OutputInstance::Loopback:
			newEndpoint = new LoopbackOutputInstance();
			break;
		case OutputInstance::RemoteOutput:
			newEndpoint = new RemoteOutputInstance(this);
			break;
		case OutputInstance::RemoteInput:
			newEndpoint = new RemoteInputInstance(this);
			break;
		case OutputInstance::DeviceOutput:
			newEndpoint = new DeviceOutputInstance(this);
			break;
		case OutputInstance::DeviceInput:
			newEndpoint = new DeviceInputInstance(this);
			break;
#ifdef _WIN32
		case OutputInstance::WasapiDeviceOutput:
			newEndpoint = new WasapiInstance(this, WasapiInstance::D_Output);
			break;
		case OutputInstance::WasapiDeviceInput:
			newEndpoint = new WasapiInstance(this, WasapiInstance::D_Input);
			break;
#endif
		case OutputInstance::None:
			return false;
			break;

		default:
			printf("Bad type %d\n", newValue);
			return false;
	}

	this->endpoint.reset(newEndpoint);

	if(oscName.isDefault()) {
		char buffer[128];
		sprintf(buffer, "waveSendUDP-osc-%s-%d", this->endpoint->getName(), outputInstance);
		oscName.setDefault(buffer);
		oscDisplayName.setDefault(buffer);
	}

	return true;
}

void OutputInstance::updateEnabledState(bool enable) {
	if(enable && !client && readyChecker.isVariablesReady()) {
		printf("Starting output: %s\n", getName().c_str());
		start();
	} else if(!enable && client) {
		printf("Stopping output: %s\n", getName().c_str());
		stop();
	}
}

void OutputInstance::onJackPropertyChangeCallback(jack_uuid_t subject,
                                                  const char* key,
                                                  jack_property_change_t change,
                                                  void* arg) {
	OutputInstance* thisInstance = (OutputInstance*) arg;
	char* pszValue = nullptr;
	char* pszType = nullptr;

	if(subject == thisInstance->clientUuid && strcmp(key, JACK_METADATA_PRETTY_NAME) == 0) {
		switch(change) {
			case PropertyCreated:
			case PropertyChanged:
				::jack_get_property(thisInstance->clientUuid, JACK_METADATA_PRETTY_NAME, &pszValue, &pszType);
				if(pszValue) {
					thisInstance->oscDisplayName = pszValue;
					::jack_free(pszValue);
				}
				if(pszType)
					::jack_free(pszType);
				break;
			case PropertyDeleted:
				thisInstance->oscDisplayName.forceDefault(thisInstance->oscName.get());
				break;
		}
		thisInstance->controlInterface->saveConfig();
	}
}

int OutputInstance::processSamplesStatic(jack_nframes_t nframes, void* arg) {
	OutputInstance* thisInstance = (OutputInstance*) arg;

	if(thisInstance->endpoint->direction == IAudioEndpoint::D_Output)
		return thisInstance->processSamples(nframes);
	else
		return thisInstance->processInputSamples(nframes);
}

int OutputInstance::processInputSamples(jack_nframes_t nframes) {
	float* buffers[32];
	float peaks[32];

	if(oscNumChannel > (int32_t)(sizeof(buffers) / sizeof(buffers[0]))) {
		printf("Too many channels, buffer too small !!!\n");
	}

	for(int32_t i = 0; i < oscNumChannel; i++) {
		buffers[i] = (jack_default_audio_sample_t*) jack_port_get_buffer(outputPorts[i], nframes);
	}

	endpoint->postProcessSamples(buffers, oscNumChannel, nframes);

	uv_mutex_lock(&filtersMutex);
	filters.processSamples(peaks, buffers, const_cast<const float**>(buffers), oscNumChannel, nframes);
	uv_mutex_unlock(&filtersMutex);

	uv_mutex_lock(&peakMutex);
	this->samplesInPeaks += nframes;
	for(int32_t i = 0; i < oscNumChannel; i++) {
		this->peaksPerChannel[i] = std::max(peaks[i], this->peaksPerChannel[i]);
	}
	uv_mutex_unlock(&peakMutex);

	return 0;
}

int OutputInstance::processSamples(jack_nframes_t nframes) {
	float* outputs[32];
	const float* inputs[32];
	float peaks[32];

	if(oscNumChannel > (int32_t)(sizeof(outputs) / sizeof(outputs[0]))) {
		printf("Too many channels, buffer too small !!!\n");
	}

	for(int32_t i = 0; i < oscNumChannel; i++) {
		inputs[i] = (jack_default_audio_sample_t*) jack_port_get_buffer(inputPorts[i], nframes);
		outputs[i] = (jack_default_audio_sample_t*) jack_port_get_buffer(outputPorts[i], nframes);
	}

	uv_mutex_lock(&filtersMutex);
	filters.processSamples(peaks, outputs, inputs, oscNumChannel, nframes);
	uv_mutex_unlock(&filtersMutex);

	// add side channel on channel 0
	if(oscNumChannel < (int32_t) inputPorts.size()) {
		jack_default_audio_sample_t* sideChannel =
		    (jack_default_audio_sample_t*) jack_port_get_buffer(inputPorts[oscNumChannel], nframes);
		for(jack_nframes_t i = 0; i < nframes; i++) {
			outputs[0][i] += filters.processSideChannelSample(sideChannel[i]);
		}
	}

	uv_mutex_lock(&peakMutex);
	this->samplesInPeaks += nframes;
	for(int32_t i = 0; i < oscNumChannel; i++) {
		this->peaksPerChannel[i] = std::max(peaks[i], this->peaksPerChannel[i]);
	}
	uv_mutex_unlock(&peakMutex);

	endpoint->postProcessSamples(outputs, oscNumChannel, nframes);

	return 0;
}

void OutputInstance::onTimeoutTimer() {
	if(!client)
		return;

	int samples;
	std::vector<float> peaks(this->peaksPerChannel.size(), 0);

	uv_mutex_lock(&peakMutex);
	samples = this->samplesInPeaks;
	peaks.swap(this->peaksPerChannel);
	this->samplesInPeaks = 0;
	uv_mutex_unlock(&peakMutex);

	if(oscSampleRate == 0)
		return;

	float deltaT = (float) samples / this->oscSampleRate;
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
		sendMessage(oscPeakGlobalPath, &argument, 1);
	}

	oscPeakPerChannelArguments.clear();
	oscPeakPerChannelArguments.reserve(levelsDb.size());
	for(auto v : levelsDb) {
		oscPeakPerChannelArguments.emplace_back(v);
	}
	sendMessage(oscPeakPerChannelPath, oscPeakPerChannelArguments.data(), oscPeakPerChannelArguments.size());

	endpoint->onTimer();
}
