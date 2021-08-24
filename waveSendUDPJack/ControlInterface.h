#ifndef CONTROLINTERFACE_H
#define CONTROLINTERFACE_H

#include "ControlClient.h"
#include "ControlServer.h"
#include "KeyBinding.h"
#include "OscRoot.h"
#include "OscServer.h"
#include "OscTcpServer.h"
#include "OutputInstance/OutputInstance.h"
#include "json.h"
#include <jack/jack.h>
#include <map>
#include <memory>
#include <uv.h>

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

	static void jackOnPortConnectStatic(jack_port_id_t a, jack_port_id_t b, int connect, void* arg);
	static int jackOnGraphReorderedStatic(void* arg);
	static void jackOnPortRegistrationStatic(jack_port_id_t port, int is_registered, void* arg);

	static void onJackNotificationStatic(uv_async_t* handle);

	void jackOnPortConnect(jack_port_id_t a, jack_port_id_t b, int connect);
	void jackOnGraphReordered();
	void jackOnPortRegistration(jack_port_id_t port, int is_registered);

private:
	int nextInstanceIndex;
	int numEq;
	std::map<int, std::unique_ptr<OutputInstance>> outputs;
	std::vector<int> outputsOrder;
	ControlServer controlServer;
	OscRoot oscRoot;
	OscServer oscUdpServer;
	OscTcpServer oscTcpServer;
	OscContainer oscRootNode;
	KeyBinding keyBinding;
	std::string saveFileName;
	jack_client_t* monitoringJackClient;

	OscReadOnlyVariable<int32_t> oscOutputNumber;
	OscEndpoint oscAddOutputInstance;
	OscEndpoint oscRemoveOutputInstance;

	struct PortConnectionStateChange {
		jack_port_id_t a;
		jack_port_id_t b;
		int connect;
	};

	struct JackNotification {
		enum Type {
			PortConnect,
			GraphReordered,
			PortRegistration,
		} type;

		union {
			struct PortConnectionStateChange portConnect;
			struct {
				jack_port_id_t port;
				int is_registered;
			} portRegistration;
		} data;
	};

	uv_async_t jackNotificationPending;
	std::mutex jackNotificationMutex;
	std::vector<JackNotification> jackNotifications;

	std::vector<PortConnectionStateChange> pendingPortChanges;
	std::map<std::string, std::set<std::string>> outputPortConnections;  // outputs to inputs
	std::map<std::string, std::set<std::string>> inputPortConnections;   // inputs from outputs
};

#endif
