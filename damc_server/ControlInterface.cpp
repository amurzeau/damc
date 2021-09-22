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
      jackPortAutoConnect(&oscRoot),
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
	std::map<std::string, std::set<std::string>> outputPortConnections;
	oscStatePersister.loadState(outputPortConnections);

	jackPortAutoConnect.start(std::move(outputPortConnections));
}

void ControlInterface::saveConfig() {
	oscStatePersister.saveState(jackPortAutoConnect.getOutputPortConnections());
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

	jackPortAutoConnect.stop();

	oscTcpServer.stop();
	oscUdpServer.stop();

	SPDLOG_INFO("Stopping audio strips jack clients");
	for(std::pair<const int, std::unique_ptr<OutputInstance>>& output : outputs) {
		output.second->stop();
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

	jackPortAutoConnect.onSlowTimer();

	SPDLOG_TRACE("Slow periodic update end");
}

void ControlInterface::releaseUvTimer(uv_timer_t* handle) {
	uv_timer_stop(handle);
	uv_close((uv_handle_t*) handle, &onCloseTimer);
}

void ControlInterface::onCloseTimer(uv_handle_t* handle) {
	delete(uv_timer_t*) handle;
}
