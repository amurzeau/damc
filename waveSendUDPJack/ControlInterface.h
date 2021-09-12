#ifndef CONTROLINTERFACE_H
#define CONTROLINTERFACE_H

#include "KeyBinding.h"
#include "OscRoot.h"
#include "OscServer.h"
#include "OscStatePersist.h"
#include "OscTcpServer.h"
#include "OutputInstance/OutputInstance.h"
#include <Osc/OscContainerArray.h>
#include <Osc/OscDynamicVariable.h>
#include <jack/jack.h>
#include <map>
#include <memory>
#include <uv.h>

class ControlInterface {
public:
	ControlInterface();
	~ControlInterface();

	int init(const char* controlIp, int controlPort);

	void run();
	void stop();

	void saveConfig();

protected:
	void loadConfig();

	static void jackOnPortConnectStatic(jack_port_id_t a, jack_port_id_t b, int connect, void* arg);
	static int jackOnGraphReorderedStatic(void* arg);
	static void jackOnPortRegistrationStatic(jack_port_id_t port, int is_registered, void* arg);

	static void onJackNotificationStatic(uv_async_t* handle);

	void jackOnPortConnect(jack_port_id_t a, jack_port_id_t b, int connect);
	void jackOnGraphReordered();
	void jackOnPortRegistration(jack_port_id_t port, int is_registered);

	static void onTimeoutTimerStatic(uv_timer_t* handle);
	void onTimeoutTimer();
	static void releaseUvTimer(uv_timer_t* handle);
	static void onCloseTimer(uv_handle_t* handle);

private:
	OscRoot oscRoot;
	OscStatePersist oscStatePersister;
	OscServer oscUdpServer;
	OscTcpServer oscTcpServer;
	OscContainerArray<OutputInstance> outputs;
	KeyBinding keyBinding;
	jack_client_t* monitoringJackClient;

	std::unique_ptr<uv_timer_t, decltype(&ControlInterface::releaseUvTimer)> updateLevelTimer;
	bool oscNeedSaveConfig;
	bool audioRunning;

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

	OscDynamicVariable<std::string> oscTypeList;
	OscDynamicVariable<std::string> oscDeviceList;
#ifdef _WIN32
	OscDynamicVariable<std::string> oscDeviceListWasapi;
#endif
};

#endif
