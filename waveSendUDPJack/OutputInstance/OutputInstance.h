#ifndef OUTPUTJACKINSTANCE_H
#define OUTPUTJACKINSTANCE_H

#include "../Filter/FilteringChain.h"
#include "../json.h"
#include "IAudioEndpoint.h"
#include <stdint.h>
#include <uv.h>
// Need to be after else stdint might conflict
#include <jack/jack.h>

class ControlServer;
class ControlClient;

class OutputInstance {
public:
	enum Type { Loopback, RemoteOutput, RemoteInput, DeviceOutput, DeviceInput, MaxType };

public:
	OutputInstance(IAudioEndpoint* endpoint);
	virtual ~OutputInstance();

	int init(ControlServer* controlServer, int type, int index, size_t numChannel, const nlohmann::json& json);
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

private:
	ControlServer* controlServer;
	IAudioEndpoint* endpoint;
	bool enabled;
	int outputInstance;
	int type;
	jack_client_t* client;
	std::string clientName;
	size_t numChannel;
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
};

#endif
