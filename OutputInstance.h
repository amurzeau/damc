#ifndef OUTPUTJACKINSTANCE_H
#define OUTPUTJACKINSTANCE_H

#include "FilteringChain.h"
#include <jack/jack.h>
#include <netinet/in.h>
#include <pthread.h>
#include <uv.h>

#include "RemoteUdpOutput.h"

class ControlServer;
class ControlClient;

class OutputInstance {
public:
	enum OutputType { Local, Remote };

public:
	OutputInstance();
	~OutputInstance();

	int init(ControlServer* controlServer,
	         OutputType outputType,
	         int index,
	         size_t numChannel,
	         size_t numEq,
	         const char* ip,
	         int port);

	int messageProcessor(const void* data, size_t size);

	void sendFilterParameters(ControlClient* client);

protected:
	static int processSamplesStatic(jack_nframes_t nframes, void* arg);
	int processSamplesLocal(jack_nframes_t nframes);
	int processSamplesRemote(jack_nframes_t nframes);

	static void onTimeoutTimerStatic(uv_timer_t* handle);
	void onTimeoutTimer();
	static void onCloseTimer(uv_handle_t* handle);

private:
	ControlServer* controlServer;
	OutputType outputType;
	int outputInstance;
	jack_client_t* client;
	char clientName[128];
	size_t numChannel;
	size_t sampleRate;
	FilterChain filters;
	std::vector<jack_port_t*> inputPorts;
	std::vector<jack_port_t*> outputPorts;
	pthread_mutex_t filtersMutex;
	RemoteUdpOutput remoteUdpOutput;

	uv_timer_t* updateLevelTimer;
	std::vector<double> levelsDb;
	pthread_mutex_t peakMutex;
	int samplesInPeaks;
	std::vector<double> peaksPerChannel;
};

#endif
