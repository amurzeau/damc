#include "ControlInterface.h"
#include "json.h"
#include <errno.h>
#include <stdio.h>
#include <uv.h>

#include <string.h>

ControlInterface::ControlInterface(const char* argv0) : numChannels(2), numEq(4) {
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

		for(const nlohmann::json& outputInstancesJson : jsonConfig.at("outputInstances")) {
			int index = outputInstancesJson.at("instance").get<int>();

			if(index >= 0 && index < (int) outputs.size()) {
				outputs[index]->setParameters(outputInstancesJson);
			}
		}
	} catch(const nlohmann::json::exception& e) {
		printf("Exception while parsing config: %s\n", e.what());
	} catch(...) {
		printf("Exception while parsing config\n");
	}
}

void ControlInterface::saveConfig() {
	nlohmann::json jsonConfigToSave = nlohmann::json::object();
	nlohmann::json outputInstancesJson = nlohmann::json::array();

	for(std::unique_ptr<OutputInstance>& output : outputs) {
		outputInstancesJson.push_back(output->getParameters());
	}

	jsonConfigToSave["outputInstances"] = outputInstancesJson;

	std::string jsonData = jsonConfigToSave.dump(4);
	std::unique_ptr<FILE, int (*)(FILE*)> file(nullptr, &fclose);

	file.reset(fopen(saveFileName.c_str(), "wb"));
	if(!file) {
		printf("Can't open save file %s: %s(%d)\n", saveFileName.c_str(), strerror(errno), errno);
	}
	fwrite(jsonData.c_str(), 1, jsonData.size(), file.get());
	fclose(file.release());
}

int ControlInterface::init(const char* controlIp, int controlPort) {
	controlServer.init(controlIp, controlPort, &messageProcessorStatic, &onNewClientStatic, this);

	return 0;
}

void ControlInterface::addLocalOutput() {
	outputs.emplace_back(new OutputInstance);
	OutputInstance* output = outputs.back().get();
	output->init(&controlServer, OutputInstance::Local, outputs.size() - 1, numChannels, numEq, nullptr, 0);
}

void ControlInterface::addRemoteOutput(const char* ip, int port) {
	outputs.emplace_back(new OutputInstance);
	OutputInstance* output = outputs.back().get();
	output->init(&controlServer, OutputInstance::Remote, outputs.size() - 1, numChannels, numEq, ip, port);
}

void ControlInterface::run() {
	controlServer.run();
}

void ControlInterface::onNewClientStatic(void* arg, ControlClient* client) {
	ControlInterface* thisInstance = (ControlInterface*) arg;
	thisInstance->onNewClient(client);
}

void ControlInterface::onNewClient(ControlClient* client) {
	nlohmann::json json = {
	    {"instance", -1}, {"numOutputInstances", outputs.size()}, {"numChannels", numChannels}, {"numEq", numEq}};

	std::string jsonStr = json.dump();

	client->sendMessage(jsonStr.c_str(), jsonStr.size());

	for(std::unique_ptr<OutputInstance>& output : outputs) {
		std::string jsonStr = output->getParameters().dump();
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

		if(outputInstance >= 0 && outputInstance < (int) outputs.size()) {
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
