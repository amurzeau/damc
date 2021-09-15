#include "ControlInterface.h"
#include "JackUtils.h"
#include <errno.h>
#include <spdlog/spdlog.h>
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
      oscStatePersister(&oscRoot, "damc_config.json"),
      oscUdpServer(&oscRoot, &oscRoot),
      oscTcpServer(&oscRoot),
      outputs(&oscRoot, "strip"),
      keyBinding(&oscRoot, &oscRoot),
      fastTimer(nullptr, &ControlInterface::releaseUvTimer),
      slowTimer(nullptr, &ControlInterface::releaseUvTimer),
      oscNeedSaveConfig(false),
      audioRunning(false),
      oscTypeList(&oscRoot, "type_list"),
      oscDeviceList(&oscRoot, "device_list")
#ifdef _WIN32
      ,
      oscDeviceListWasapi(&oscRoot, "device_list_wasapi")
#endif
{
	SPDLOG_INFO("Starting control interface");

	outputs.setFactory(
	    [this](OscContainer* parent, int name) { return new OutputInstance(parent, this, name, audioRunning); });

	jack_status_t status;

	SPDLOG_INFO("Initializing async Jack notification handler");
	jackNotificationPending.data = this;
	uv_async_init(uv_default_loop(), &jackNotificationPending, &ControlInterface::onJackNotificationStatic);
	uv_unref((uv_handle_t*) &jackNotificationPending);

	std::string monitoringClientName = OutputInstance::JACK_CLIENT_NAME_PREFIX + "monitoringclient";
	SPDLOG_INFO("Opening monitoring jack client {}", monitoringClientName);

	monitoringJackClient = jack_client_open(monitoringClientName.c_str(), JackNullOption, &status);
	if(monitoringJackClient != nullptr) {
		jack_set_port_connect_callback(monitoringJackClient, &ControlInterface::jackOnPortConnectStatic, this);
		jack_set_graph_order_callback(monitoringJackClient, &ControlInterface::jackOnGraphReorderedStatic, this);
		jack_set_port_registration_callback(
		    monitoringJackClient, &ControlInterface::jackOnPortRegistrationStatic, this);
		jack_activate(monitoringJackClient);
		SPDLOG_INFO("Started monitoring jack client");
	} else {
		SPDLOG_ERROR("Failed to start monitoring jack client: {}", status);
		JackUtils::logJackStatus(spdlog::level::err, status);

		return;
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

	oscRoot.setOnOscValueChanged([this]() {
		if(!oscNeedSaveConfig)
			SPDLOG_DEBUG("OSC value changed, request deferred config save");
		oscNeedSaveConfig = true;
	});

	initializeTimer(fastTimer, "fast periodic timer", 66, &ControlInterface::onFastTimerStatic);    // 15fps
	initializeTimer(slowTimer, "slow periodic timer", 1000, &ControlInterface::onSlowTimerStatic);  // 1fps
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
	oscStatePersister.saveState(outputPortConnections);
}

void ControlInterface::initializeTimer(std::unique_ptr<uv_timer_t, void (*)(uv_timer_t*)>& timer,
                                       const char* name,
                                       uint64_t period_ms,
                                       void (*callback)(uv_timer_t*)) {
	SPDLOG_INFO("Initializing {}", name);
	timer.reset(new uv_timer_t);
	uv_timer_init(uv_default_loop(), timer.get());
	timer->data = this;
	uv_timer_start(timer.get(), callback, period_ms, period_ms);
	uv_unref((uv_handle_t*) timer.get());
	SPDLOG_INFO("Started {}", name);
}

int ControlInterface::init(const char* controlIp, int controlPort) {
	if(monitoringJackClient == nullptr) {
		return -1;
	}

	loadConfig();

	SPDLOG_INFO("Activating {} audio strips", outputs.size());
	audioRunning = true;
	for(std::pair<const int, std::unique_ptr<OutputInstance>>& output : outputs) {
		output.second->activate();
	}

	SPDLOG_INFO("Starting UDP server");
	oscUdpServer.init(controlIp, controlPort + 1, "127.0.0.1", 10000);

	SPDLOG_INFO("Starting TCP server");
	oscTcpServer.init(controlIp, controlPort + 2);

	return 0;
}

void ControlInterface::run() {
	SPDLOG_INFO("Running...");
	uv_run(uv_default_loop(), UV_RUN_DEFAULT);
}

void ControlInterface::stop() {
	SPDLOG_INFO("Stopping server");

	if(monitoringJackClient) {
		SPDLOG_INFO("Stopping monitoring client");
		jack_deactivate(monitoringJackClient);
		jack_client_close(monitoringJackClient);
	}

	oscTcpServer.stop();
	oscUdpServer.stop();

	SPDLOG_INFO("Stopping audio strips jack clients");
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

	SPDLOG_TRACE("jack event: port connect: {} <=> {} {}", a, b, connect ? "connected" : "disconnected");

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

	SPDLOG_TRACE("jack event: graph reordered");

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

	SPDLOG_TRACE("jack event: port registration: {} {}", port, is_registered ? "registered" : "unregistered");

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

	SPDLOG_INFO("Processing {} jack notifications", pendingNotifications.size());

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

	SPDLOG_INFO("Processing {} port connection changes", portChanges.size());

	for(PortConnectionStateChange& portChange : portChanges) {
		jack_port_t* portA = jack_port_by_id(monitoringJackClient, portChange.a);
		jack_port_t* portB = jack_port_by_id(monitoringJackClient, portChange.b);

		if(portA == nullptr || portB == nullptr) {
			SPDLOG_TRACE("Port disconnected because client was disconnected (port are not available anymore)");
			continue;
		}

		const char* aName = jack_port_name(portA);
		const char* bName = jack_port_name(portB);
		if(aName == nullptr || bName == nullptr) {
			SPDLOG_TRACE("Port disconnected because client was disconnected (port have no name anymore)");
			continue;
		}

		jack_port_t* portAByName = jack_port_by_name(monitoringJackClient, aName);
		jack_port_t* portBByName = jack_port_by_name(monitoringJackClient, bName);

		SPDLOG_TRACE("port {}:\n  a = {}, name: {}, byname = {}\n  b = {}, name: {}, byname = {}",
		             portChange.connect ? "connected" : "disconnected",
		             (void*) portA,
		             aName,
		             portAByName,
		             (void*) portB,
		             bName,
		             portBByName);

		if(portAByName == nullptr || portBByName == nullptr) {
			// There was a disconnect because a client disconnected
			// Don't save it
			SPDLOG_TRACE("Port disconnected because client was disconnected (can't find port by name anymore)");
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
			SPDLOG_TRACE("Not a output to input connection between {} and {}, ignore", aName, bName);
			continue;
		}

		if(portChange.connect) {
			SPDLOG_DEBUG("Port connection {} <=> {}", outputPort, inputPort);

			outputPortConnections[outputPort].insert(inputPort);
			inputPortConnections[inputPort].insert(outputPort);
			oscNeedSaveConfig = true;
		} else {
			SPDLOG_DEBUG("Port disconnection {} <=> {}", outputPort, inputPort);

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

	SPDLOG_TRACE("Processing port connection update for port {}", port);

	jack_port_t* portPtr = jack_port_by_id(monitoringJackClient, port);
	if(!portPtr) {
		SPDLOG_TRACE("Port {} registered but can't open it", port);
		return;
	}

	int portFlags = jack_port_flags(portPtr);

	const char* portName = jack_port_name(portPtr);
	if(!portName) {
		SPDLOG_TRACE("Port {} registered but has no name", port);
		return;
	}

	SPDLOG_TRACE("Port {} registered", portName);

	std::map<std::string, std::set<std::string>>* portConnections;

	if(portFlags & JackPortIsOutput) {
		portConnections = &outputPortConnections;
	} else if(portFlags & JackPortIsInput) {
		portConnections = &inputPortConnections;
	} else {
		SPDLOG_TRACE("Port {} not an input or output", portName);
		return;
	}

	auto it = portConnections->find(portName);

	if(it == portConnections->end()) {
		SPDLOG_TRACE("Port {} not in list of ports to auto-connect", portName);
		return;
	}

	for(const auto& inputPorts : it->second) {
		if(portConnections == &inputPortConnections) {
			SPDLOG_DEBUG("Autoconnect {} to {}", inputPorts, it->first);
			jack_connect(monitoringJackClient, inputPorts.c_str(), it->first.c_str());
		} else {
			SPDLOG_DEBUG("Autoconnect {} to {}", it->first, inputPorts);
			jack_connect(monitoringJackClient, it->first.c_str(), inputPorts.c_str());
		}
	}
}

void ControlInterface::onFastTimerStatic(uv_timer_t* handle) {
	ControlInterface* thisInstance = (ControlInterface*) handle->data;
	thisInstance->onFastTimer();
}

void ControlInterface::onSlowTimerStatic(uv_timer_t* handle) {
	ControlInterface* thisInstance = (ControlInterface*) handle->data;
	thisInstance->onSlowTimer();
}

void ControlInterface::onFastTimer() {
	for(auto& outputInstance : outputs) {
		outputInstance.second->onFastTimer();
	}
}

void ControlInterface::onSlowTimer() {
	SPDLOG_TRACE("Slow periodic update");

	for(auto& outputInstance : outputs) {
		outputInstance.second->onSlowTimer();
	}

	if(oscNeedSaveConfig) {
		oscNeedSaveConfig = false;
		saveConfig();
	}

	SPDLOG_TRACE("Slow periodic update end");
}

void ControlInterface::releaseUvTimer(uv_timer_t* handle) {
	uv_timer_stop(handle);
	uv_close((uv_handle_t*) handle, &onCloseTimer);
}

void ControlInterface::onCloseTimer(uv_handle_t* handle) {
	delete(uv_timer_t*) handle;
}
