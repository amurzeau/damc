#ifndef OUTPUTJACKINSTANCE_H
#define OUTPUTJACKINSTANCE_H

#include "../Filter/FilteringChain.h"
#include "OscAddress.h"
#include "../OscServer.h"
#include "../json.h"
#include "IAudioEndpoint.h"
#include <stdint.h>
#include <uv.h>
// Need to be after else stdint might conflict
#include <jack/jack.h>
#include <jack/metadata.h>

class ControlInterface;
class ControlServer;
class ControlClient;

class OutputInstance : public OscContainer {
public:
	enum Type {
		Loopback,
		RemoteOutput,
		RemoteInput,
		DeviceOutput,
		DeviceInput,
		WasapiDeviceOutput,
		WasapiDeviceInput,
		MaxType
	};

public:
	OutputInstance(OscContainer* parent, size_t index, IAudioEndpoint* endpoint);
	virtual ~OutputInstance();

	int init(ControlInterface* controlInterface,
	         ControlServer* controlServer,
	         OscServer* oscServer,
	         int type,
	         size_t numChannel,
	         const nlohmann::json& json);
	int start();
	void stop();

	void setParameters(const nlohmann::json& json);
	nlohmann::json getParameters();

protected:
	static int processSamplesStatic(jack_nframes_t nframes, void* arg);
	int processInputSamples(jack_nframes_t nframes);
	int processSamples(jack_nframes_t nframes);

	static void onTimeoutTimerStatic(uv_timer_t* handle);
	void onTimeoutTimer();
	static void onCloseTimer(uv_handle_t* handle);

	static void onJackPropertyChangeCallback(jack_uuid_t subject,
	                                         const char* key,
	                                         jack_property_change_t change,
	                                         void* arg);

private:
	ControlInterface* controlInterface;
	ControlServer* controlServer;
	OscServer* oscServer;
	IAudioEndpoint* endpoint;
	OscVariable<bool> enabled;
	OscReadOnlyVariable<int32_t> outputInstance;
	OscReadOnlyVariable<int32_t> type;
	jack_client_t* client;
	jack_uuid_t clientUuid;
	OscVariable<std::string> clientName;
	OscVariable<std::string> clientDisplayName;
	OscReadOnlyVariable<int32_t> numChannel;
	size_t sampleRate;
	FilterChain filters;
	std::vector<jack_port_t*> inputPorts;
	std::vector<jack_port_t*> outputPorts;
	uv_mutex_t filtersMutex;

	nlohmann::json controlSettings;

	uv_timer_t* updateLevelTimer;
	std::vector<float> levelsDb;
	uv_mutex_t peakMutex;
	int samplesInPeaks;
	std::vector<float> peaksPerChannel;
	OscReadOnlyVariable<float> oscPeakGlobal;
	std::string oscPeakPerChannelPath;
	OscVariable<bool> oscEnablePeakUpdate;
	OscVariable<bool> oscEnablePeakJsonUpdate;
};

#endif
