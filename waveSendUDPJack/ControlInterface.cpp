#include "ControlInterface.h"
#include <errno.h>
#include <stdio.h>
#include <uv.h>

#include <set>
#include <string.h>

#include "OutputInstance/DeviceInputInstance.h"
#include "OutputInstance/DeviceOutputInstance.h"
#include "OutputInstance/LoopbackOutputInstance.h"
#include "OutputInstance/RemoteInputInstance.h"
#include "OutputInstance/RemoteOutputInstance.h"
#ifdef _WIN32
#include "OutputInstance/WasapiInstance.h"
#endif

ControlInterface::ControlInterface(const char* argv0)
    : nextInstanceIndex(0), numEq(6), oscRootNode(&oscServer, "strip"), keyBinding(&oscServer) {
	char basePath[260];

	size_t n = sizeof(basePath);
	if(uv_exepath(basePath, &n) == 0)
		basePath[n] = '\0';
	else
		basePath[0] = '\0';

	if(basePath[0] == 0) {
		saveFileName = "waveSendUDPJack.json";
	} else {
		size_t len = strlen(basePath);
		char* p = basePath + len;
		while(p >= basePath) {
			if(*p == '/' || *p == '\\')
				break;
			p--;
		}
		if(p >= basePath)
			*p = 0;

		saveFileName = std::string(basePath) + "/waveSendUDPJack.json";
	}

	jack_status_t status;

	jackNotificationPending.data = this;
	uv_async_init(uv_default_loop(), &jackNotificationPending, &ControlInterface::onJackNotificationStatic);
	uv_unref((uv_handle_t*) &jackNotificationPending);

	monitoringJackClient = jack_client_open("wavePlayUDP-monitoringclient", JackNullOption, &status);
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
}

ControlInterface::~ControlInterface() {}

void ControlInterface::loadConfig() {
	std::unique_ptr<FILE, int (*)(FILE*)> file(nullptr, &fclose);
	std::string jsonData;

	printf("Loading config file %s\n", saveFileName.c_str());

	file.reset(fopen(saveFileName.c_str(), "rb"));
	if(!file) {
		printf("Can't open config file, ignoring\n");
		return;
	}

	fseek(file.get(), 0, SEEK_END);
	jsonData.resize(ftell(file.get()));
	fseek(file.get(), 0, SEEK_SET);

	fread(&jsonData[0], 1, jsonData.size(), file.get());
	fclose(file.release());

	try {
		nlohmann::json jsonConfig = nlohmann::json::parse(jsonData);

		outputsOrder = jsonConfig.value("outputsOrder", std::vector<int>{});
		outputPortConnections = jsonConfig.value("portConnections", std::map<std::string, std::set<std::string>>{});

		for(const nlohmann::json& outputInstancesJson : jsonConfig.at("outputInstances")) {
			addOutputInstance(outputInstancesJson);
		}

		// Construct inputPortConnections from outputPortConnections
		for(const auto& outputPort : outputPortConnections) {
			for(const auto& inputPort : outputPort.second) {
				inputPortConnections[inputPort].insert(outputPort.first);
			}
		}
	} catch(const nlohmann::json::exception& e) {
		printf("Exception while parsing config: %s\n", e.what());
		printf("json: %s\n", jsonData.c_str());
	} catch(...) {
		printf("Exception while parsing config\n");
	}

	// oscServer.printAllNodes();
}

void ControlInterface::saveConfig() {
	nlohmann::json jsonConfigToSave = nlohmann::json::object();
	nlohmann::json outputInstancesJson = nlohmann::json::array();

	try {
		for(auto& output : outputs) {
			outputInstancesJson.push_back(output.second->getParameters());
		}

		jsonConfigToSave["outputsOrder"] = outputsOrder;
		jsonConfigToSave["outputInstances"] = outputInstancesJson;
		jsonConfigToSave["portConnections"] = outputPortConnections;

		std::string jsonData = jsonConfigToSave.dump(4);
		std::unique_ptr<FILE, int (*)(FILE*)> file(nullptr, &fclose);

		file.reset(fopen(saveFileName.c_str(), "wb"));
		if(!file) {
			printf("Can't open save file %s: %s(%d)\n", saveFileName.c_str(), strerror(errno), errno);
		}
		fwrite(jsonData.c_str(), 1, jsonData.size(), file.get());
		fclose(file.release());
	} catch(const nlohmann::json::exception& e) {
		printf("Exception while saving config: %s\n", e.what());
	} catch(...) {
		printf("Exception while saving config\n");
	}
}

std::map<int, std::unique_ptr<OutputInstance>>::iterator ControlInterface::addOutputInstance(
    const nlohmann::json& outputInstancesJson) {
	int type = outputInstancesJson.value("type", -1);
	int instance = outputInstancesJson.value("instance", -1);
	int numChannel;
	std::unique_ptr<OutputInstance> outputInstance;

	if(instance == -1)
		instance = nextInstanceIndex;
	else if(nextInstanceIndex < instance)
		nextInstanceIndex = instance;

	nextInstanceIndex++;

	switch(type) {
		case OutputInstance::Loopback:
			outputInstance.reset(new OutputInstance(&oscRootNode, instance, new LoopbackOutputInstance));
			break;
		case OutputInstance::RemoteOutput:
			outputInstance.reset(new OutputInstance(&oscRootNode, instance, new RemoteOutputInstance));
			break;
		case OutputInstance::RemoteInput:
			outputInstance.reset(new OutputInstance(&oscRootNode, instance, new RemoteInputInstance));
			break;
		case OutputInstance::DeviceOutput:
			outputInstance.reset(new OutputInstance(&oscRootNode, instance, new DeviceOutputInstance));
			break;
		case OutputInstance::DeviceInput:
			outputInstance.reset(new OutputInstance(&oscRootNode, instance, new DeviceInputInstance));
			break;
#ifdef _WIN32
		case OutputInstance::WasapiDeviceOutput:
			outputInstance.reset(
			    new OutputInstance(&oscRootNode, instance, new WasapiInstance(WasapiInstance::D_Output)));
			break;
		case OutputInstance::WasapiDeviceInput:
			outputInstance.reset(
			    new OutputInstance(&oscRootNode, instance, new WasapiInstance(WasapiInstance::D_Input)));
			break;
#endif
		default:
			printf("Bad type %d\n", type);
			return outputs.end();
	}

	auto numChannelElement = outputInstancesJson.find("numChannels");
	if(numChannelElement != outputInstancesJson.end()) {
		numChannel = numChannelElement.value().get<int>();
	} else {
		printf("Missing numChannel configuration, using 2\n");
		numChannel = 2;
	}
	outputInstance->init(this, &controlServer, &oscServer, type, numChannel, outputInstancesJson);

	return outputs.emplace(std::make_pair(instance, std::move(outputInstance))).first;
}

void ControlInterface::removeOutputInstance(std::map<int, std::unique_ptr<OutputInstance>>::iterator index) {
	index->second->stop();
	outputs.erase(index);
}

int ControlInterface::init(const char* controlIp, int controlPort) {
	controlServer.init(controlIp, controlPort, &messageProcessorStatic, &onNewClientStatic, this);
	oscServer.init(controlIp, controlPort + 1);

	return 0;
}

void ControlInterface::run() {
	controlServer.run();
}

void ControlInterface::stop() {
	if(monitoringJackClient) {
		printf("Stopping monitoring client\n");
		jack_deactivate(monitoringJackClient);
		jack_client_close(monitoringJackClient);
	}

	oscServer.stop();
	controlServer.stop();

	for(std::pair<const int, std::unique_ptr<OutputInstance>>& output : outputs) {
		output.second->stop();
	}
}

void ControlInterface::onNewClientStatic(void* arg, ControlClient* client) {
	ControlInterface* thisInstance = (ControlInterface*) arg;
	thisInstance->onNewClient(client);
}

void ControlInterface::onNewClient(ControlClient* client) {
	nlohmann::json json = {
	    {"instance", -1}, {"operation", "outputList"}, {"numOutputInstances", outputs.size()}, {"numEq", numEq}};

	std::string jsonStr = json.dump();

	client->sendMessage(jsonStr.c_str(), jsonStr.size());

	std::set<int> outputsToSend;
	for(auto& output : outputs) {
		outputsToSend.insert(output.first);
	}

	for(int index : outputsOrder) {
		auto outputIt = outputsToSend.find(index);
		if(outputIt == outputsToSend.end())
			continue;

		auto& output = outputs[index];

		nlohmann::json json = {{"instance", -1}, {"operation", "add"}, {"target", index}};
		std::string jsonStr = json.dump();
		client->sendMessage(jsonStr.c_str(), jsonStr.size());

		jsonStr = output->getParameters().dump();
		client->sendMessage(jsonStr.c_str(), jsonStr.size());

		outputsToSend.erase(outputIt);
	}

	// Send remaning outputs that were not referenced by outputsOrder
	for(int index : outputsToSend) {
		auto& output = outputs[index];

		nlohmann::json json = {{"instance", -1}, {"operation", "add"}, {"target", index}};
		std::string jsonStr = json.dump();
		client->sendMessage(jsonStr.c_str(), jsonStr.size());

		jsonStr = output->getParameters().dump();
		client->sendMessage(jsonStr.c_str(), jsonStr.size());
	}
}

void ControlInterface::messageProcessorStatic(void* arg, const void* data, size_t size) {
	ControlInterface* thisInstance = (ControlInterface*) arg;
	thisInstance->messageProcessor(data, size);
}

void ControlInterface::messageProcessor(const void* data, size_t size) {
	nlohmann::json json;

	try {
		json = nlohmann::json::parse((const char*) data, (const char*) data + size);
		int outputInstance = json.at("instance").get<int>();

		if(outputInstance == -1) {
			printf("Received command: %s\n", json.dump().c_str());
			std::string operation = json.value("operation", "none");
			int target = json.value("target", -1);

			if(operation == "add") {
				auto it = addOutputInstance(json);

				if(it != outputs.end()) {
					json["target"] = it->first;
					std::string jsonStr = json.dump();
					controlServer.sendMessage(jsonStr.c_str(), jsonStr.size());

					jsonStr = it->second->getParameters().dump();
					controlServer.sendMessage(jsonStr.c_str(), jsonStr.size());
					printf("Added interface %d\n", it->first);
					saveConfig();
				}
			} else if(operation == "remove") {
				auto it = outputs.find(target);
				if(it == outputs.end()) {
					printf("Bad delete target %d\n", target);
				} else {
					removeOutputInstance(it);

					std::string jsonStr = json.dump();
					controlServer.sendMessage(jsonStr.c_str(), jsonStr.size());
					printf("Removed interface %d\n", target);
					saveConfig();
				}
			} else if(operation == "query") {
				nlohmann::json json = {{"instance", -1},
				                       {"operation", "queryResult"},
				                       {"deviceList", DeviceOutputInstance::getDeviceList()},
#ifdef _WIN32
				                       {"deviceListWasapi", WasapiInstance::getDeviceList()},
#endif
				                       {"typeList",
				                        nlohmann::json::array({"Loopback",
				                                               "RemoteOutput",
				                                               "RemoteInput",
				                                               "DeviceOutput",
				                                               "DeviceInput",
				                                               "WasapiOutput",
				                                               "WasapiInput"})}};
				std::string jsonStr = json.dump();
				controlServer.sendMessage(jsonStr.c_str(), jsonStr.size());
			} else if(operation == "outputsOrder") {
				outputsOrder = json.value("outputsOrder", std::vector<int>{});
				saveConfig();
			}
		} else if(outputs.count(outputInstance) > 0) {
			outputs[outputInstance]->setParameters(json);

			std::string jsonStr = json.dump();
			controlServer.sendMessage(jsonStr.c_str(), jsonStr.size());

			saveConfig();
		} else {
			printf("Bad output instance %d\n", outputInstance);
		}
	} catch(const nlohmann::json::exception& e) {
		printf("Exception while parsing message: %s\n", e.what());
	} catch(...) {
		printf("Exception while parsing message\n");
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
			// printf("Port connection %s to %s\n", outputPort, inputPort);
		} else {
			// printf("Port disconnection %s to %s\n", outputPort, inputPort);
			if(outputPortConnections.count(outputPort)) {
				outputPortConnections[outputPort].erase(inputPort);
				if(outputPortConnections[outputPort].empty())
					outputPortConnections.erase(outputPort);
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

	if(it == portConnections->end())
		return;

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
