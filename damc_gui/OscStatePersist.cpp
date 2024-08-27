#include "OscStatePersist.h"
#include "OscRoot.h"
#include <exception>
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <uv.h>

#include "json.h"

OscStatePersist::OscStatePersist(OscRoot* oscRoot) : oscRoot(oscRoot) {}

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

void OscStatePersist::loadState(std::string filename) {
	std::unique_ptr<FILE, int (*)(FILE*)> file(nullptr, &fclose);
	std::string jsonData;
	std::map<std::string, std::vector<OscArgument>> configValues;

	SPDLOG_INFO("Loading config file {}", filename);

	file.reset(fopen(filename.c_str(), "rb"));
	if(!file) {
		throw std::runtime_error(
		    fmt::format("Can't open config file {}, skipping config load ({} ({}))", filename, strerror(errno), errno));
	}

	fseek(file.get(), 0, SEEK_END);
	jsonData.resize(ftell(file.get()));
	fseek(file.get(), 0, SEEK_SET);

	fread(&jsonData[0], 1, jsonData.size(), file.get());
	fclose(file.release());

	SPDLOG_TRACE("Config file read: {}", jsonData);

	nlohmann::json jsonConfig = nlohmann::json::parse(jsonData);
	recurseJson(&configValues, "", jsonConfig);

	SPDLOG_DEBUG("Updating OSC variables");
	oscRoot->loadNodeConfig(configValues);

	SPDLOG_DEBUG("Loaded config");
}

void OscStatePersist::saveState(std::string filename) {
	std::unique_ptr<FILE, int (*)(FILE*)> file(nullptr, &fclose);
	std::string jsonData;

	SPDLOG_INFO("Saving config");

	jsonData = oscRoot->getAsString();

	SPDLOG_DEBUG("Config file to write: {}", jsonData);

	nlohmann::json jsonConfig = nlohmann::json::parse(jsonData);

	jsonData = jsonConfig.dump(4);

	file.reset(fopen(filename.c_str(), "wb"));
	if(!file) {
		throw std::runtime_error(
		    fmt::format("Can't open config file for saving {}, error {} ({})", filename, strerror(errno), errno));
	}

	fwrite(jsonData.c_str(), 1, jsonData.size(), file.get());
	fclose(file.release());

	SPDLOG_DEBUG("Saved config to {}", filename);
}
