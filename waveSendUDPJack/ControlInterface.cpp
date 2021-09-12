#include "ControlInterface.h"
#include <errno.h>
#include <stdio.h>
#include <uv.h>

#include "Utils.h"
#include <set>
#include <string.h>

#include "OutputInstance/DeviceOutputInstance.h"
#ifdef _WIN32
#include "OutputInstance/WasapiInstance.h"
#endif

ControlInterface::ControlInterface()
    : oscRoot(true),
      oscStatePersister(&oscRoot, "waveSendUDPJack_osc.json"),
      oscUdpServer(&oscRoot, &oscRoot),
      oscTcpServer(&oscRoot),
      outputs(&oscRoot, "strip"),
      keyBinding(&oscRoot, &oscRoot),
      updateLevelTimer(nullptr, &ControlInterface::releaseUvTimer),
      oscNeedSaveConfig(false),
      audioRunning(false),
      oscTypeList(&oscRoot, "type_list"),
      oscDeviceList(&oscRoot, "device_list")
#ifdef _WIN32
      ,
      oscDeviceListWasapi(&oscRoot, "device_list_wasapi")
#endif
{
	outputs.setFactory(
	    [this](OscContainer* parent, int name) { return new OutputInstance(parent, this, name, audioRunning); });

	jack_status_t status;

	jackNotificationPending.data = this;
	uv_async_init(uv_default_loop(), &jackNotificationPending, &ControlInterface::onJackNotificationStatic);
	uv_unref((uv_handle_t*) &jackNotificationPending);

	monitoringJackClient = jack_client_open("waveSendUDP-monitoringclient", JackNullOption, &status);
	if(monitoringJackClient != nullptr) {
		printf("Started monitoring client\n");
		jack_set_port_connect_callback(monitoringJackClient, &ControlInterface::jackOnPortConnectStatic, this);
		jack_set_graph_order_callback(monitoringJackClient, &ControlInterface::jackOnGraphReorderedStatic, this);
		jack_set_port_registration_callback(
		    monitoringJackClient, &ControlInterface::jackOnPortRegistrationStatic, this);
		jack_activate(monitoringJackClient);
	} else {
		printf("Failed to start monitoring client: %d\n", status);
	}

	oscTypeList.setReadCallback([]() {
		return std::vector<std::string>{{
#define STRING_ITEM(item) #item,
		    OUTPUT_INSTANCE_TYPES(STRING_ITEM)
#undef STRING_ITEM
		}};
	});
	oscDeviceList.setReadCallback([]() { return DeviceOutputInstance::getDeviceList(); });
#ifdef _WIN32
	oscDeviceListWasapi.setReadCallback([]() { return WasapiInstance::getDeviceList(); });
#endif

	oscRoot.setOnOscValueChanged([this]() { oscNeedSaveConfig = true; });

	updateLevelTimer.reset(new uv_timer_t);
	uv_timer_init(uv_default_loop(), updateLevelTimer.get());
	updateLevelTimer->data = this;
	uv_timer_start(updateLevelTimer.get(), &onTimeoutTimerStatic, 66, 66);  // 15fps
	uv_unref((uv_handle_t*) updateLevelTimer.get());
}

ControlInterface::~ControlInterface() {}

void ControlInterface::loadConfig() {
	oscStatePersister.loadState(outputPortConnections);

	// oscRoot.printAllNodes();

	// Construct inputPortConnections from outputPortConnections
	for(const auto& outputPort : outputPortConnections) {
		for(const auto& inputPort : outputPort.second) {
			inputPortConnections[inputPort].insert(outputPort.first);
		}
	}
}

void ControlInterface::saveConfig() {
	printf("Saving config\n");
	oscStatePersister.saveState(outputPortConnections);
}

int ControlInterface::init(const char* controlIp, int controlPort) {
	loadConfig();

	audioRunning = true;
	for(std::pair<const int, std::unique_ptr<OutputInstance>>& output : outputs) {
		output.second->activate();
	}

	oscUdpServer.init(controlIp, controlPort + 1);
	oscTcpServer.init(controlIp, controlPort + 2);

	return 0;
}

void ControlInterface::run() {
	uv_run(uv_default_loop(), UV_RUN_DEFAULT);
}

void ControlInterface::stop() {
	if(monitoringJackClient) {
		printf("Stopping monitoring client\n");
		jack_deactivate(monitoringJackClient);
		jack_client_close(monitoringJackClient);
	}

	oscTcpServer.stop();
	oscUdpServer.stop();

	for(std::pair<const int, std::unique_ptr<OutputInstance>>& output : outputs) {
		output.second->stop();
	}
}

void ControlInterface::jackOnPortConnectStatic(jack_port_id_t a, jack_port_id_t b, int connect, void* arg) {
	ControlInterface* thisInstance = (ControlInterface*) arg;

	JackNotification notification;
	notification.type = JackNotification::PortConnect;
	notification.data.portConnect.a = a;
	notification.data.portConnect.b = b;
	notification.data.portConnect.connect = connect;

	{
		std::lock_guard guard(thisInstance->jackNotificationMutex);
		thisInstance->jackNotifications.push_back(notification);
	}
	uv_async_send(&thisInstance->jackNotificationPending);
}

int ControlInterface::jackOnGraphReorderedStatic(void* arg) {
	ControlInterface* thisInstance = (ControlInterface*) arg;

	JackNotification notification;
	notification.type = JackNotification::GraphReordered;

	{
		std::lock_guard guard(thisInstance->jackNotificationMutex);
		thisInstance->jackNotifications.push_back(notification);
	}
	uv_async_send(&thisInstance->jackNotificationPending);

	return 0;
}

void ControlInterface::jackOnPortRegistrationStatic(jack_port_id_t port, int is_registered, void* arg) {
	ControlInterface* thisInstance = (ControlInterface*) arg;

	JackNotification notification;
	notification.type = JackNotification::PortRegistration;
	notification.data.portRegistration.port = port;
	notification.data.portRegistration.is_registered = is_registered;

	{
		std::lock_guard guard(thisInstance->jackNotificationMutex);
		thisInstance->jackNotifications.push_back(notification);
	}
	uv_async_send(&thisInstance->jackNotificationPending);
}

void ControlInterface::onJackNotificationStatic(uv_async_t* handle) {
	ControlInterface* thisInstance = (ControlInterface*) handle->data;
	std::vector<JackNotification> pendingNotifications;

	{
		std::lock_guard guard(thisInstance->jackNotificationMutex);
		pendingNotifications.swap(thisInstance->jackNotifications);
	}

	for(const auto& notification : pendingNotifications) {
		switch(notification.type) {
			case JackNotification::PortConnect:
				thisInstance->jackOnPortConnect(notification.data.portConnect.a,
				                                notification.data.portConnect.b,
				                                notification.data.portConnect.connect);
				break;
			case JackNotification::GraphReordered:
				thisInstance->jackOnGraphReordered();
				break;
			case JackNotification::PortRegistration:
				thisInstance->jackOnPortRegistration(notification.data.portRegistration.port,
				                                     notification.data.portRegistration.is_registered);
				break;
		}
	}
}

void ControlInterface::jackOnPortConnect(jack_port_id_t a, jack_port_id_t b, int connect) {
	PortConnectionStateChange portChange;
	portChange.a = a;
	portChange.b = b;
	portChange.connect = connect;

	pendingPortChanges.push_back(portChange);
}

void ControlInterface::jackOnGraphReordered() {
	std::vector<PortConnectionStateChange> portChanges;

	pendingPortChanges.swap(portChanges);

	for(PortConnectionStateChange& portChange : portChanges) {
		jack_port_t* portA = jack_port_by_id(monitoringJackClient, portChange.a);
		jack_port_t* portB = jack_port_by_id(monitoringJackClient, portChange.b);

		if(portA == nullptr || portB == nullptr) {
			// There was a disconnect because a client disconnected
			// Don't save it
			continue;
		}

		const char* aName = jack_port_name(portA);
		const char* bName = jack_port_name(portB);
		if(aName == nullptr || bName == nullptr) {
			// There was a disconnect because a client disconnected
			// Don't save it
			continue;
		}

		jack_port_t* portAByName = jack_port_by_name(monitoringJackClient, aName);
		jack_port_t* portBByName = jack_port_by_name(monitoringJackClient, bName);

		// printf("port %s:\n  a = %p, name: %s, byname = %p\n  b = %p, name: %s, byname = %p\n",
		//        portChange.connect ? "connected" : "disconnected",
		//        portA,
		//        aName,
		//        portAByName,
		//        portB,
		//        bName,
		//        portBByName);

		if(portAByName == nullptr || portBByName == nullptr) {
			// There was a disconnect because a client disconnected
			// Don't save it
			continue;
		}

		const char* inputPort;
		const char* outputPort;

		int portAFlags = jack_port_flags(portAByName);
		int portBFlags = jack_port_flags(portBByName);

		if((portAFlags & JackPortIsOutput) != 0 && (portBFlags & JackPortIsInput) != 0) {
			inputPort = bName;
			outputPort = aName;
		} else if((portAFlags & JackPortIsOutput) != 0 && (portBFlags & JackPortIsInput) != 0) {
			inputPort = aName;
			outputPort = bName;
		} else {
			printf("Not a output to input connection between %s and %s, ignore\n", aName, bName);
			continue;
		}

		if(portChange.connect) {
			outputPortConnections[outputPort].insert(inputPort);
			inputPortConnections[inputPort].insert(outputPort);
			oscNeedSaveConfig = true;
			// printf("Port connection %s to %s\n", outputPort, inputPort);
		} else {
			// printf("Port disconnection %s to %s\n", outputPort, inputPort);
			if(outputPortConnections.count(outputPort)) {
				outputPortConnections[outputPort].erase(inputPort);
				if(outputPortConnections[outputPort].empty())
					outputPortConnections.erase(outputPort);
				oscNeedSaveConfig = true;
			}
			if(inputPortConnections.count(inputPort)) {
				inputPortConnections[inputPort].erase(outputPort);
				if(inputPortConnections[inputPort].empty())
					inputPortConnections.erase(inputPort);
			}
		}
	}
}

void ControlInterface::jackOnPortRegistration(jack_port_id_t port, int is_registered) {
	if(!is_registered)
		return;

	jack_port_t* portPtr = jack_port_by_id(monitoringJackClient, port);
	if(!portPtr)
		return;

	int portFlags = jack_port_flags(portPtr);

	const char* portName = jack_port_name(portPtr);
	if(!portName)
		return;
	// printf("Port %s registered\n", portName);

	std::map<std::string, std::set<std::string>>* portConnections;

	if(portFlags & JackPortIsOutput) {
		portConnections = &outputPortConnections;
	} else if(portFlags & JackPortIsInput) {
		portConnections = &inputPortConnections;
	} else {
		printf("Port %s not an input or output\n", portName);
		return;
	}

	auto it = portConnections->find(portName);

	if(it == portConnections->end()) {
		// printf("Port %s not in list\n", portName);
		return;
	}

	for(const auto& inputPorts : it->second) {
		if(portConnections == &inputPortConnections) {
			// printf("Autoconnect %s to %s\n", inputPorts.c_str(), it->first.c_str());
			jack_connect(monitoringJackClient, inputPorts.c_str(), it->first.c_str());
		} else {
			// printf("Autoconnect %s to %s\n", it->first.c_str(), inputPorts.c_str());
			jack_connect(monitoringJackClient, it->first.c_str(), inputPorts.c_str());
		}
	}
}

void ControlInterface::onTimeoutTimerStatic(uv_timer_t* handle) {
	ControlInterface* thisInstance = (ControlInterface*) handle->data;
	thisInstance->onTimeoutTimer();
}

void ControlInterface::onTimeoutTimer() {
	for(auto& outputInstance : outputs) {
		outputInstance.second->onTimeoutTimer();
	}
	if(oscNeedSaveConfig) {
		oscNeedSaveConfig = false;
		saveConfig();
	}
}

void ControlInterface::releaseUvTimer(uv_timer_t* handle) {
	uv_timer_stop(handle);
	uv_close((uv_handle_t*) handle, &onCloseTimer);
}

void ControlInterface::onCloseTimer(uv_handle_t* handle) {
	delete(uv_timer_t*) handle;
}
