#ifndef CONTROLINTERFACE_H
#define CONTROLINTERFACE_H

#include "ControlClient.h"
#include "ControlServer.h"
#include "OutputInstance.h"
#include <memory>
#include <vector>

class ControlInterface {
public:
	ControlInterface();
	~ControlInterface();

	int init(const char* controlIp, int controlPort);

	void addLocalOutput();
	void addRemoteOutput(const char* ip, int port);

	void run();

protected:
	static void onNewClientStatic(void* arg, ControlClient* client);
	void onNewClient(ControlClient* client);

	static void messageProcessorStatic(void* arg, const void* data, size_t size);
	void messageProcessor(const void* data, size_t size);

private:
	int numChannels;
	int numEq;
	std::vector<std::unique_ptr<OutputInstance>> outputs;
	ControlServer controlServer;
};

#endif
