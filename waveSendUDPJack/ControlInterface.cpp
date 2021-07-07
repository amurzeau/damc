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

ControlInterface::ControlInterface(const char* argv0) : nextInstanceIndex(0), numEq(6) {
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
		for(const nlohmann::json& outputInstancesJson : jsonConfig.at("outputInstances")) {
			addOutputInstance(outputInstancesJson);
		}
	} catch(const nlohmann::json::exception& e) {
		printf("Exception while parsing config: %s\n", e.what());
		printf("json: %s\n", jsonData.c_str());
	} catch(...) {
		printf("Exception while parsing config\n");
	}
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

	switch(type) {
		case OutputInstance::Loopback:
			outputInstance.reset(new OutputInstance(new LoopbackOutputInstance));
			break;
		case OutputInstance::RemoteOutput:
			outputInstance.reset(new OutputInstance(new RemoteOutputInstance));
			break;
		case OutputInstance::RemoteInput:
			outputInstance.reset(new OutputInstance(new RemoteInputInstance));
			break;
		case OutputInstance::DeviceOutput:
			outputInstance.reset(new OutputInstance(new DeviceOutputInstance));
			break;
		case OutputInstance::DeviceInput:
			outputInstance.reset(new OutputInstance(new DeviceInputInstance));
			break;
#ifdef _WIN32
		case OutputInstance::WasapiDeviceOutput:
			outputInstance.reset(new OutputInstance(new WasapiInstance(WasapiInstance::D_Output)));
			break;
		case OutputInstance::WasapiDeviceInput:
			outputInstance.reset(new OutputInstance(new WasapiInstance(WasapiInstance::D_Input)));
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

	if(instance == -1)
		instance = nextInstanceIndex;
	else if(nextInstanceIndex < instance)
		nextInstanceIndex = instance;

	nextInstanceIndex++;
	outputInstance->init(this, &controlServer, type, instance, numChannel, outputInstancesJson);

	return outputs.emplace(std::make_pair(instance, std::move(outputInstance))).first;
}

void ControlInterface::removeOutputInstance(std::map<int, std::unique_ptr<OutputInstance>>::iterator index) {
	index->second->stop();
	outputs.erase(index);
}

int ControlInterface::init(const char* controlIp, int controlPort) {
	controlServer.init(controlIp, controlPort, &messageProcessorStatic, &onNewClientStatic, this);

	return 0;
}

void ControlInterface::run() {
	controlServer.run();
}

void ControlInterface::stop() {
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
