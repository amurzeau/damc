#include "OutputInstance.h"
#include "../ControlInterface.h"
#include <algorithm>
#include <jack/metadata.h>
#include <jack/uuid.h>
#include <spdlog/spdlog.h>

#include "DeviceInputInstance.h"
#include "DeviceOutputInstance.h"
#include "LoopbackOutputInstance.h"
#include "RemoteInputInstance.h"
#include "RemoteOutputInstance.h"
#ifdef _WIN32
#include "WasapiInstance.h"
#endif

const std::string OutputInstance::JACK_CLIENT_NAME_PREFIX = "damc-";
const std::string OutputInstance::JACK_CLIENT_DISPLAYNAME_PREFIX = "damc: ";

OutputInstance::OutputInstance(OscContainer* parent, ControlInterface* controlInterface, int index, bool audioRunning)
    : OscContainer(parent, std::to_string(index)),
      outputInstance(index),
      controlInterface(controlInterface),
      client(nullptr),
      filters(this),
      jackSampleRateMeasure(this, "realSampleRate"),

      enableAudio(false),
      oscEnable(this, "enable", false),
      oscType(this, "_type", (int32_t) OutputInstance::None),  // use _type to ensure it is before endpoint config
      oscName(this, "name", std::to_string(index)),
      oscDisplayName(this, "display_name", oscName.get()),
      oscNumChannel(this, "channels", 2),
      oscSampleRate(this, "sample_rate"),

      oscEnablePeakUpdate(this, "enable_peak", false),
      oscEnablePeakJsonUpdate(this, "enable_peak_json", true),
      displayNameUpdateRequested(false) {
	uv_mutex_init(&filtersMutex);
	uv_mutex_init(&peakMutex);

	oscPeakGlobalPath = getFullAddress() + "/meter";
	oscPeakPerChannelPath = getFullAddress() + "/meter_per_channel";

	oscType.addCheckCallback([this](int newValue) -> bool {
		if(client) {
			SPDLOG_ERROR("Can't change type when output is enabled");
			return false;
		}

		switch(newValue) {
#define ENUM_ITEM(item) case OutputInstance::item:
			OUTPUT_INSTANCE_TYPES(ENUM_ITEM)
#undef ENUM_ITEM
			break;

			default:
				SPDLOG_ERROR("Bad type {}", newValue);
				return false;
		}

		return true;
	});

	oscType.setChangeCallback([this](int newValue) { updateType(newValue); });

	oscName.addCheckCallback([this](const std::string&) -> bool {
		if(client) {
			SPDLOG_ERROR("Can't change jack name when output is enabled");
			return false;
		}
		return true;
	});

	oscDisplayName.setChangeCallback([this](const std::string&) { displayNameUpdateRequested = true; });

	oscNumChannel.addCheckCallback([this](int32_t newValue) {
		if(client) {
			SPDLOG_ERROR("Can't change channel number when output is enabled");
			return false;
		}

		if(newValue > 32) {
			SPDLOG_ERROR("Too many channels: {}", newValue);
			return false;
		}

		if(newValue <= 0) {
			SPDLOG_ERROR("Invalid value: {}", newValue);
			return false;
		}

		return true;
	});

	oscNumChannel.setChangeCallback([this](int32_t newValue) {
		SPDLOG_INFO("Changing channel number to {}", newValue);

		levelsDb.resize(newValue, -192);
		peaksPerChannel.resize(newValue, 0);
		samplesInPeaks = 0;

		filters.init(newValue);
	});

	if(audioRunning) {
		activate();
	}
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

void OutputInstance::activate() {
	enableAudio = true;
	oscEnable.addCheckCallback([this](bool newValue) {
		if(newValue) {
			if(!endpoint) {
				SPDLOG_ERROR("Can't enable {} ({}): type is None", oscDisplayName.get(), oscName.get());
				return false;
			}
			if(oscNumChannel <= 0) {
				SPDLOG_ERROR("Can't enable {} ({}): channel number is <= 0: {}",
				             oscDisplayName.get(),
				             oscName.get(),
				             oscNumChannel.get());
				return false;
			}
		}
		return true;
	});
	oscEnable.setChangeCallback([this](bool newValue) { updateEnabledState(newValue); });
}

int OutputInstance::start() {
	jack_status_t status;

	if(client || !endpoint)
		return 0;

	std::string jackClientName = JACK_CLIENT_NAME_PREFIX + oscName.get();
	SPDLOG_INFO("Opening jack client {}", jackClientName.c_str());
	client = jack_client_open(jackClientName.c_str(), JackNullOption, &status);
	if(client == NULL) {
		SPDLOG_ERROR("Failed to open jack: {}", status);
		return -3;
	}

	SPDLOG_INFO("Opened jack client");

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
			SPDLOG_ERROR("cannot register input port \"{}\"!", name);
			jack_client_close(client);
			client = nullptr;
			return -4;
		}

		sprintf(name, "output_%d", (int) (i + 1));
		outputPorts[i] =
		    jack_port_register(client, name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput | additionnalPortFlags, 0);
		if(outputPorts[i] == 0) {
			SPDLOG_ERROR("cannot register output port \"{}\"!", name);
			jack_client_close(client);
			client = nullptr;
			return -4;
		}
	}

	if(oscType != DeviceInput && oscType != RemoteInput) {
		inputPorts.push_back(jack_port_register(
		    client, "side_channel", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput | additionnalPortFlags, 0));
		if(inputPorts.back() == 0) {
			SPDLOG_ERROR("cannot register input port \"{}\"!", "side_channel");
			jack_client_close(client);
			client = nullptr;
			return -4;
		}
	}

	endpoint->start(outputInstance, oscNumChannel, jackSampleRate, jack_get_buffer_size(client));

	jack_set_process_callback(client, &processSamplesStatic, this);

	const char* pszClientUuid = ::jack_get_uuid_for_client_name(client, jackClientName.c_str());
	if(pszClientUuid) {
		::jack_uuid_parse(pszClientUuid, &clientUuid);
		jack_free((void*) pszClientUuid);
	} else {
		clientUuid = 0;
	}
	updateJackDisplayName();
	jack_set_property_change_callback(client, &OutputInstance::onJackPropertyChangeCallback, this);

	int ret = jack_activate(client);
	if(ret) {
		SPDLOG_ERROR("cannot activate client: {}", ret);
	}

	SPDLOG_INFO("Processing interface {}...", outputInstance);

	return 0;
}

void OutputInstance::stop() {
	SPDLOG_INFO("Stopping jack client {}", oscName.get());

	if(client) {
		jack_deactivate(client);
		jack_client_close(client);
		client = nullptr;

		if(endpoint)
			endpoint->stop();
	}
}

bool OutputInstance::updateType(int newValue) {
	IAudioEndpoint* newEndpoint = nullptr;

	// Reset before to avoid having duplicate OSC addresses
	this->endpoint.reset(nullptr);

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
			newEndpoint = nullptr;
			break;

		default:
			SPDLOG_ERROR("Bad type {}", newValue);
			return false;
	}

	this->endpoint.reset(newEndpoint);

	return true;
}

void OutputInstance::updateEnabledState(bool newValue) {
	if(!enableAudio) {
		SPDLOG_DEBUG("Not enabling audio yet, will do after config load");
		return;
	}

	if(newValue && !client && endpoint) {
		SPDLOG_INFO("Starting output: {}", getName());
		start();
	} else if(!newValue && client) {
		SPDLOG_INFO("Stopping output: {}", getName());
		stop();
	}
}

void OutputInstance::updateJackDisplayName() {
	if(clientUuid == 0) {
		SPDLOG_DEBUG("Not updating jack client pretty name: no uuid");
		return;
	}

	if(!oscDisplayName.get().empty()) {
		std::string prettyName = JACK_CLIENT_DISPLAYNAME_PREFIX + oscDisplayName.get();
		SPDLOG_DEBUG("Setting {} jack display name to {}", oscName.get(), prettyName);
		::jack_set_property(client, clientUuid, JACK_METADATA_PRETTY_NAME, prettyName.c_str(), NULL);
	} else {
		SPDLOG_DEBUG("Setting display name to default value {}", oscName.get());
		// This will trigger updateJackDisplayName again with the new value updated
		oscDisplayName.forceDefault(oscName.get());
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
					std::string newValue = pszValue;
					size_t prefixPos = newValue.find(JACK_CLIENT_DISPLAYNAME_PREFIX);
					if(prefixPos == 0) {
						newValue.replace(prefixPos, JACK_CLIENT_DISPLAYNAME_PREFIX.size(), "");
						SPDLOG_INFO("Setting {} display name to {}", thisInstance->oscName.get(), newValue);
						thisInstance->oscDisplayName = newValue;
					} else {
						// Invalid name, keep previous
						SPDLOG_WARN("Invalid name, missing prefix {}: {}", JACK_CLIENT_DISPLAYNAME_PREFIX, pszValue);
						thisInstance->displayNameUpdateRequested = true;
					}
					::jack_free(pszValue);
				}
				if(pszType)
					::jack_free(pszType);
				break;
			case PropertyDeleted:
				SPDLOG_DEBUG("Jack display name deleted, setting display name to default value {}",
				             thisInstance->oscName.get());
				thisInstance->oscDisplayName.forceDefault(thisInstance->oscName.get());
				break;
		}
	}
}

int OutputInstance::processSamplesStatic(jack_nframes_t nframes, void* arg) {
	OutputInstance* thisInstance = (OutputInstance*) arg;

	thisInstance->jackSampleRateMeasure.notifySampleProcessed(nframes);

	if(thisInstance->endpoint->direction == IAudioEndpoint::D_Output)
		return thisInstance->processSamples(nframes);
	else
		return thisInstance->processInputSamples(nframes);
}

int OutputInstance::processInputSamples(jack_nframes_t nframes) {
	float* buffers[32];
	float peaks[32];

	if(oscNumChannel > (int32_t)(sizeof(buffers) / sizeof(buffers[0]))) {
		SPDLOG_ERROR("Too many channels, buffer too small !!!");
		return 0;
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
		SPDLOG_ERROR("Too many channels, buffer too small !!!");
		return 0;
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

void OutputInstance::onFastTimer() {
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
		getRoot()->sendMessage(oscPeakGlobalPath, &argument, 1);
	}

	oscPeakPerChannelArguments.clear();
	oscPeakPerChannelArguments.reserve(levelsDb.size());
	for(auto v : levelsDb) {
		oscPeakPerChannelArguments.emplace_back(v);
	}
	getRoot()->sendMessage(oscPeakPerChannelPath, oscPeakPerChannelArguments.data(), oscPeakPerChannelArguments.size());

	if(endpoint)
		endpoint->onFastTimer();

	if(displayNameUpdateRequested) {
		displayNameUpdateRequested = false;
		// Don't call that function directly as displayName can be update in Jack notification thread
		// where we can't call jack functions
		updateJackDisplayName();
	}
}

void OutputInstance::onSlowTimer() {
	if(!client)
		return;

	if(endpoint)
		endpoint->onSlowTimer();

	jackSampleRateMeasure.onTimeoutTimer();
}
