#include "OscStatePersist.h"
#include "OscRoot.h"
#include <uv.h>

#include "json.h"

OscStatePersist::OscStatePersist(OscRoot* oscRoot, std::string fileName) : oscRoot(oscRoot) {
	char basePath[1024];

	size_t n = sizeof(basePath);
	if(uv_exepath(basePath, &n) == 0)
		basePath[n] = '\0';
	else
		basePath[0] = '\0';

	if(basePath[0] == 0) {
		saveFileName = fileName;
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

		saveFileName = std::string(basePath) + "/" + fileName;
	}
}

static void execute(std::map<std::string, std::vector<OscArgument>>* configValues,
                    const std::string& address,
                    const std::vector<OscArgument>& arguments) {
	configValues->insert(std::make_pair(address, arguments));
}

static void recurseJson(std::map<std::string, std::vector<OscArgument>>* configValues,
                        const std::string& address,
                        const nlohmann::json& object) {
	std::vector<OscArgument> values;

	switch(object.type()) {
			// OscContainer and derived
		case nlohmann::json::value_t::object:
			for(const auto& property : object.items()) {
				std::string objectAddress;
				objectAddress = address + "/" + property.key();
				recurseJson(configValues, objectAddress, property.value());
			}
			break;

			// OscFlatArray
		case nlohmann::json::value_t::array: {
			bool skipExecute = false;
			std::vector<OscArgument> values;
			for(const auto& property : object.items()) {
				OscArgument addArgument = property.key();
				switch(property.value().type()) {
					case nlohmann::json::value_t::string:
						values.push_back(property.value().get<std::string>());
						break;

					case nlohmann::json::value_t::boolean:
						values.push_back(property.value().get<bool>());
						break;

					case nlohmann::json::value_t::number_integer:
					case nlohmann::json::value_t::number_unsigned:
						values.push_back(property.value().get<int32_t>());
						break;

					case nlohmann::json::value_t::number_float:
						values.push_back(property.value().get<float>());
						break;

					case nlohmann::json::value_t::null:
						throw std::invalid_argument("json array " + address + " contains unsupported type null");
						break;
					case nlohmann::json::value_t::object:
						throw std::invalid_argument("json array " + address + " contains unsupported type object");
						break;
					case nlohmann::json::value_t::array:
						throw std::invalid_argument("json array " + address + " contains unsupported type array");
						break;
					case nlohmann::json::value_t::binary:
						throw std::invalid_argument("json array " + address + " contains unsupported type binary");
						break;
					case nlohmann::json::value_t::discarded:
						throw std::invalid_argument("json array " + address + " contains unsupported type discarded");
						break;
				}
			}
			if(!skipExecute) {
				execute(configValues, address, values);
			}
			break;
		}

			// OscVariable
		case nlohmann::json::value_t::string:
			values.push_back(object.get<std::string>());
			break;

		case nlohmann::json::value_t::boolean:
			values.push_back(object.get<bool>());
			break;

		case nlohmann::json::value_t::number_integer:
		case nlohmann::json::value_t::number_unsigned:
			values.push_back(object.get<int32_t>());
			break;

		case nlohmann::json::value_t::number_float:
			values.push_back(object.get<float>());
			break;

		case nlohmann::json::value_t::null:
			throw std::invalid_argument("json array " + address + " contains unsupported type null");
			break;
		case nlohmann::json::value_t::binary:
			throw std::invalid_argument("json array " + address + " contains unsupported type binary");
			break;
		case nlohmann::json::value_t::discarded:
			throw std::invalid_argument("json array " + address + " contains unsupported type discarded");
			break;
	}
	if(!values.empty()) {
		execute(configValues, address, values);
	}
}

void OscStatePersist::loadState(std::map<std::string, std::set<std::string>>& outputPortConnections) {
	std::unique_ptr<FILE, int (*)(FILE*)> file(nullptr, &fclose);
	std::string jsonData;
	std::map<std::string, std::vector<OscArgument>> configValues;

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
		recurseJson(&configValues, "", jsonConfig);

		outputPortConnections = jsonConfig.value("portConnections", std::map<std::string, std::set<std::string>>{});

		oscRoot->loadNodeConfig(configValues);
	} catch(const std::exception& e) {
		printf("Exception while parsing config: %s\n", e.what());
		printf("json: %s\n", jsonData.c_str());
	} catch(...) {
		printf("Exception while parsing config\n");
	}
}

void OscStatePersist::saveState(const std::map<std::string, std::set<std::string>>& outputPortConnections) {
	std::unique_ptr<FILE, int (*)(FILE*)> file(nullptr, &fclose);
	std::string jsonData = oscRoot->getAsString();

	try {
		nlohmann::json jsonConfig = nlohmann::json::parse(jsonData);
		jsonConfig["portConnections"] = outputPortConnections;

		jsonData = jsonConfig.dump(4);

		file.reset(fopen(saveFileName.c_str(), "wb"));
		if(!file) {
			printf("Can't open save file %s: %s(%d)\n", saveFileName.c_str(), strerror(errno), errno);
			return;
		}
		fwrite(jsonData.c_str(), 1, jsonData.size(), file.get());
		fclose(file.release());
	} catch(const std::exception& e) {
		printf("Exception while parsing config: %s\n", e.what());
		printf("json: %s\n", jsonData.c_str());
	} catch(...) {
		printf("Exception while parsing config\n");
	}
}
