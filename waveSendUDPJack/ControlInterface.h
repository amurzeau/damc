#ifndef CONTROLINTERFACE_H
#define CONTROLINTERFACE_H

#include "ControlClient.h"
#include "ControlServer.h"
#include "OutputInstance/OutputInstance.h"
#include "json.h"
#include <map>
#include <memory>

class ControlInterface {
public:
	ControlInterface(const char* argv0);
	~ControlInterface();

	int init(const char* controlIp, int controlPort);

	void run();
	void stop();

	void loadConfig();
	void saveConfig();

	std::map<int, std::unique_ptr<OutputInstance>>::iterator addOutputInstance(
	    const nlohmann::json& outputInstancesJson);
	void removeOutputInstance(std::map<int, std::unique_ptr<OutputInstance>>::iterator index);

protected:
	static void onNewClientStatic(void* arg, ControlClient* client);
	void onNewClient(ControlClient* client);

	static void messageProcessorStatic(void* arg, const void* data, size_t size);
	void messageProcessor(const void* data, size_t size);

private:
	int nextInstanceIndex;
	int numEq;
	std::map<int, std::unique_ptr<OutputInstance>> outputs;
	std::vector<int> outputsOrder;
	ControlServer controlServer;
	std::string saveFileName;
};

#endif
