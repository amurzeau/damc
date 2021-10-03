#pragma once

#include "../Filter/FilteringChain.h"
#include "IAudioEndpoint.h"
#include "SampleRateMeasure.h"
#include <Osc/OscCombinedVariable.h>
#include <Osc/OscContainer.h>
#include <stdint.h>
#include <uv.h>

// Need to be after else stdint might conflict
#include <jack/jack.h>
#include <jack/metadata.h>

class ControlInterface;

class ChannelStrip : public OscContainer {
public:
#ifdef _WIN32
#define WASAPI_TYPES(_) \
	_(WasapiDeviceOutput) \
	_(WasapiDeviceInput)
#else
#define WASAPI_TYPES(_)
#endif

#define OUTPUT_INSTANCE_VALID_TYPES(_) \
	_(Loopback) \
	_(RemoteOutput) \
	_(RemoteInput) \
	_(DeviceOutput) \
	_(DeviceInput) \
	WASAPI_TYPES(_)

#define OUTPUT_INSTANCE_TYPES(_) \
	OUTPUT_INSTANCE_VALID_TYPES(_) \
	_(None)

	enum Type {
#define ENUM_ITEM(item) item,
		OUTPUT_INSTANCE_TYPES(ENUM_ITEM)
#undef ENUM_ITEM
		    MaxType
	};

	static const std::string JACK_CLIENT_NAME_PREFIX;
	static const std::string JACK_CLIENT_DISPLAYNAME_PREFIX;

public:
	ChannelStrip(OscContainer* parent, ControlInterface* controlInterface, int index, bool audioRunning);
	virtual ~ChannelStrip();

	void activate();
	int start();
	void stop();
	void onFastTimer();
	void onSlowTimer();

protected:
	bool updateType(int newValue);
	void updateEnabledState(bool enable);

	static int processSamplesStatic(jack_nframes_t nframes, void* arg);
	int processInputSamples(jack_nframes_t nframes);
	int processSamples(jack_nframes_t nframes);

	void updateJackDisplayName();
	static void onJackPropertyChangeCallback(jack_uuid_t subject,
	                                         const char* key,
	                                         jack_property_change_t change,
	                                         void* arg);

private:
	const int outputInstance;
	ControlInterface* controlInterface;
	std::unique_ptr<IAudioEndpoint> endpoint;
	jack_client_t* client;
	jack_uuid_t clientUuid;
	int jackSampleRate;
	std::vector<jack_port_t*> inputPorts;
	std::vector<jack_port_t*> outputPorts;

	SampleRateMeasure jackSampleRateMeasure;

	bool enableAudio;
	OscVariable<bool> oscEnable;
	OscVariable<int32_t> oscType;
	OscVariable<std::string> oscName;
	OscVariable<std::string> oscDisplayName;
	OscVariable<int32_t> oscNumChannel;
	OscReadOnlyVariable<int32_t> oscSampleRate;

	FilterChain filters;

	bool displayNameUpdateRequested;
};
