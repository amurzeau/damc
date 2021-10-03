#pragma once

#include <Osc/OscVariable.h>
#include <jack/types.h>
#include <map>
#include <mutex>
#include <set>
#include <string>
#include <vector>

class OscContainer;

class JackPortAutoConnect {
public:
	JackPortAutoConnect(OscContainer* oscParent);
	~JackPortAutoConnect();

	void start(std::map<std::string, std::set<std::string>> outputPortConnections);
	void stop();
	void onSlowTimer();

	const std::map<std::string, std::set<std::string>>& getOutputPortConnections() { return outputPortConnections; }

protected:
	static void jackOnPortConnectStatic(jack_port_id_t a, jack_port_id_t b, int connect, void* arg);
	static int jackOnGraphReorderedStatic(void* arg);
	static void jackOnPortRegistrationStatic(jack_port_id_t port, int is_registered, void* arg);

	void processJackNotifications();

	void jackOnPortConnect(jack_port_id_t a, jack_port_id_t b, int connect);
	void jackOnGraphReordered();
	void jackOnPortRegistration(jack_port_id_t port, int is_registered);

	void saveAllPortConnections();
	void autoConnectAllExistingPorts();

	void savePortConnection(const char* aName, const char* bName, bool connect);
	void autoConnectPort(const char* portName);

private:
	jack_client_t* monitoringJackClient;

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

	OscRoot* oscRoot;
	OscVariable<bool> oscEnableAutoConnect;
	OscVariable<bool> oscEnableConnectMonitoring;

	std::mutex jackNotificationMutex;
	std::vector<JackNotification> jackNotifications;
	size_t previousNotificationCount = 0;

	std::vector<PortConnectionStateChange> pendingPortChanges;
	std::map<std::string, std::set<std::string>> outputPortConnections;  // outputs to inputs
	std::map<std::string, std::set<std::string>> inputPortConnections;   // inputs from outputs
};
