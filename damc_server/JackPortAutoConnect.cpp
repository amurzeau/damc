#include "JackPortAutoConnect.h"
#include "JackUtils.h"
#include "OutputInstance/OutputInstance.h"
#include <OscRoot.h>
#include <jack/jack.h>
#include <spdlog/spdlog.h>

JackPortAutoConnect::JackPortAutoConnect(OscContainer* oscParent)
    : oscRoot(oscParent->getRoot()),
      oscEnableAutoConnect(oscParent, "enableAutoConnect", true),
      oscEnableConnectMonitoring(oscParent, "enableConnectionMonitoring", true) {
	jack_status_t status;

	SPDLOG_INFO("Initializing async Jack notification handler");
	jackNotificationPending.data = this;
	uv_async_init(uv_default_loop(), &jackNotificationPending, &JackPortAutoConnect::onJackNotificationStatic);
	uv_unref((uv_handle_t*) &jackNotificationPending);

	std::string monitoringClientName = OutputInstance::JACK_CLIENT_NAME_PREFIX + "monitoringclient";
	SPDLOG_INFO("Opening monitoring jack client {}", monitoringClientName);

	monitoringJackClient = jack_client_open(monitoringClientName.c_str(), JackNullOption, &status);
	if(monitoringJackClient != nullptr) {
		jack_set_port_connect_callback(monitoringJackClient, &JackPortAutoConnect::jackOnPortConnectStatic, this);
		jack_set_graph_order_callback(monitoringJackClient, &JackPortAutoConnect::jackOnGraphReorderedStatic, this);
		jack_set_port_registration_callback(
		    monitoringJackClient, &JackPortAutoConnect::jackOnPortRegistrationStatic, this);
		jack_activate(monitoringJackClient);
		SPDLOG_INFO("Started monitoring jack client");
	} else {
		SPDLOG_ERROR("Failed to start monitoring jack client: {}", status);
		JackUtils::logJackStatus(spdlog::level::err, status);
	}
}

JackPortAutoConnect::~JackPortAutoConnect() {
	stop();
}

void JackPortAutoConnect::start(std::map<std::string, std::set<std::string>> outputPortConnections) {
	if(!monitoringJackClient)
		return;

	SPDLOG_INFO("Starting jack port connection monitor");

	this->outputPortConnections = std::move(outputPortConnections);

	// Construct inputPortConnections from outputPortConnections
	for(const auto& outputPort : this->outputPortConnections) {
		for(const auto& inputPort : outputPort.second) {
			inputPortConnections[inputPort].insert(outputPort.first);
		}
	}

	jack_activate(monitoringJackClient);

	// Save already present connections
	saveAllPortConnections();

	// Connect already present ports
	autoConnectAllExistingPorts();

	oscEnableConnectMonitoring.setChangeCallback([this](bool newValue) {
		if(newValue) {
			saveAllPortConnections();
		}
	});
	oscEnableAutoConnect.setChangeCallback([this](bool newValue) {
		if(newValue) {
			autoConnectAllExistingPorts();
		}
	});
}

void JackPortAutoConnect::stop() {
	if(monitoringJackClient) {
		SPDLOG_INFO("Stopping monitoring client");
		jack_deactivate(monitoringJackClient);
		jack_client_close(monitoringJackClient);
		monitoringJackClient = nullptr;
	}
}

void JackPortAutoConnect::jackOnPortConnectStatic(jack_port_id_t a, jack_port_id_t b, int connect, void* arg) {
	JackPortAutoConnect* thisInstance = (JackPortAutoConnect*) arg;

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

int JackPortAutoConnect::jackOnGraphReorderedStatic(void* arg) {
	JackPortAutoConnect* thisInstance = (JackPortAutoConnect*) arg;

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

void JackPortAutoConnect::jackOnPortRegistrationStatic(jack_port_id_t port, int is_registered, void* arg) {
	JackPortAutoConnect* thisInstance = (JackPortAutoConnect*) arg;

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

void JackPortAutoConnect::onJackNotificationStatic(uv_async_t* handle) {
	JackPortAutoConnect* thisInstance = (JackPortAutoConnect*) handle->data;
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

void JackPortAutoConnect::jackOnPortConnect(jack_port_id_t a, jack_port_id_t b, int connect) {
	PortConnectionStateChange portChange;
	portChange.a = a;
	portChange.b = b;
	portChange.connect = connect;

	pendingPortChanges.push_back(portChange);
}

void JackPortAutoConnect::jackOnGraphReordered() {
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

		savePortConnection(aName, bName, portChange.connect);
	}
}

void JackPortAutoConnect::jackOnPortRegistration(jack_port_id_t port, int is_registered) {
	if(!is_registered)
		return;

	SPDLOG_TRACE("Processing port connection update for port {}", port);

	jack_port_t* portPtr = jack_port_by_id(monitoringJackClient, port);
	if(!portPtr) {
		SPDLOG_TRACE("Port {} registered but can't open it", port);
		return;
	}

	const char* portName = jack_port_name(portPtr);
	if(!portName) {
		SPDLOG_TRACE("Port {} registered but has no name", port);
		return;
	}

	SPDLOG_TRACE("Port {} registered", portName);
	autoConnectPort(portName);
}

void JackPortAutoConnect::saveAllPortConnections() {
	if(!oscEnableConnectMonitoring)
		return;

	SPDLOG_INFO("Saving already made connections");

	const char** const ports = jack_get_ports(monitoringJackClient, nullptr, nullptr, 0);

	// Save already connected ports
	for(const char** port = ports; port && *port != nullptr; port++) {
		jack_port_t* portPtr = jack_port_by_name(monitoringJackClient, *port);

		int portFlags = jack_port_flags(portPtr);

		// Only process outputs to avoid processing connections 2 times
		if(!(portFlags & JackPortIsOutput))
			continue;

		const char** const connections = jack_port_get_all_connections(monitoringJackClient, portPtr);

		for(const char** connection = connections; connection && *connection != nullptr; connection++) {
			savePortConnection(*port, *connection, true);
		}

		jack_free(connections);
	}

	jack_free(ports);
}

void JackPortAutoConnect::autoConnectAllExistingPorts() {
	SPDLOG_INFO("Connecting jack clients");

	const char** const ports = jack_get_ports(monitoringJackClient, nullptr, nullptr, 0);

	// Connect already present ports
	for(const char** port = ports; port && *port != nullptr; port++) {
		SPDLOG_INFO("AutoConnecting port {}", *port);
		autoConnectPort(*port);
	}

	jack_free(ports);
}

void JackPortAutoConnect::savePortConnection(const char* aName, const char* bName, bool connect) {
	if(!oscEnableConnectMonitoring)
		return;

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
		return;
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
		return;
	}

	if(connect) {
		SPDLOG_DEBUG("Port connection {} <=> {}", outputPort, inputPort);

		if(outputPortConnections[outputPort].insert(inputPort).second)
			oscRoot->notifyValueChanged();

		inputPortConnections[inputPort].insert(outputPort);
	} else {
		SPDLOG_DEBUG("Port disconnection {} <=> {}", outputPort, inputPort);

		if(outputPortConnections.count(outputPort)) {
			outputPortConnections[outputPort].erase(inputPort);
			if(outputPortConnections[outputPort].empty())
				outputPortConnections.erase(outputPort);
			oscRoot->notifyValueChanged();
		}
		if(inputPortConnections.count(inputPort)) {
			inputPortConnections[inputPort].erase(outputPort);
			if(inputPortConnections[inputPort].empty())
				inputPortConnections.erase(inputPort);
		}
	}
}

void JackPortAutoConnect::autoConnectPort(const char* portName) {
	std::map<std::string, std::set<std::string>>* portConnections;

	if(!oscEnableAutoConnect) {
		SPDLOG_TRACE("Autoconnection disabled, not connecting {}", port);
		return;
	}

	jack_port_t* port = jack_port_by_name(monitoringJackClient, portName);
	if(!port) {
		SPDLOG_TRACE("Port {} registered but can't open it", port);
		return;
	}

	int portFlags = jack_port_flags(port);

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
