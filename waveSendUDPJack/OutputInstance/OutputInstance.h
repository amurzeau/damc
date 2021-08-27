#ifndef OUTPUTJACKINSTANCE_H
#define OUTPUTJACKINSTANCE_H

#include "../Filter/FilteringChain.h"
#include "IAudioEndpoint.h"
#include <Osc/OscCombinedVariable.h>
#include <Osc/OscContainer.h>
#include <stdint.h>
#include <uv.h>

// Need to be after else stdint might conflict
#include <jack/jack.h>
#include <jack/metadata.h>

class ControlInterface;

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
		None,
		MaxType
	};

public:
	OutputInstance(OscContainer* parent, ControlInterface* controlInterface, int index);
	virtual ~OutputInstance();

	int start();
	void stop();
	void onTimeoutTimer();

protected:
	bool updateType(int newValue);
	void updateEnabledState(bool enable);

	static int processSamplesStatic(jack_nframes_t nframes, void* arg);
	int processInputSamples(jack_nframes_t nframes);
	int processSamples(jack_nframes_t nframes);

	static void onJackPropertyChangeCallback(jack_uuid_t subject,
	                                         const char* key,
	                                         jack_property_change_t change,
	                                         void* arg);

private:
	int outputInstance;
	ControlInterface* controlInterface;
	std::unique_ptr<IAudioEndpoint> endpoint;
	jack_client_t* client;
	jack_uuid_t clientUuid;
	int jackSampleRate;
	FilterChain filters;
	std::vector<jack_port_t*> inputPorts;
	std::vector<jack_port_t*> outputPorts;
	uv_mutex_t filtersMutex;

	std::vector<float> levelsDb;
	uv_mutex_t peakMutex;
	int samplesInPeaks;
	std::vector<float> peaksPerChannel;
	std::string oscPeakGlobalPath;
	std::string oscPeakPerChannelPath;
	std::vector<OscArgument> oscPeakPerChannelArguments;

	OscVariable<bool> oscEnable;
	OscVariable<int32_t> oscType;
	OscVariable<std::string> oscName;
	OscVariable<std::string> oscDisplayName;
	OscVariable<int32_t> oscNumChannel;
	OscVariable<int32_t> oscSampleRate;

	OscVariable<bool> oscEnablePeakUpdate;
	OscVariable<bool> oscEnablePeakJsonUpdate;

	OscCombinedVariable readyChecker;
};

#endif
