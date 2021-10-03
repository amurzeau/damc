#pragma once

#include "ChannelStrip/ChannelStrip.h"
#include "JackPortAutoConnect.h"
#include "KeyBinding.h"
#include "OscRoot.h"
#include "OscServer.h"
#include "OscStatePersist.h"
#include "OscTcpServer.h"
#include <Osc/OscContainerArray.h>
#include <Osc/OscDynamicVariable.h>
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
	void initializeTimer(std::unique_ptr<uv_timer_t, void (*)(uv_timer_t*)>& timer,
	                     const char* name,
	                     uint64_t period_ms,
	                     void (*callback)(uv_timer_t*));
	void loadConfig();

	static void onFastTimerStatic(uv_timer_t* handle);
	static void onSlowTimerStatic(uv_timer_t* handle);
	void onFastTimer();
	void onSlowTimer();
	static void releaseUvTimer(uv_timer_t* handle);
	static void onCloseTimer(uv_handle_t* handle);

private:
	OscRoot oscRoot;
	OscStatePersist oscStatePersister;
	OscServer oscUdpServer;
	OscTcpServer oscTcpServer;
	OscContainerArray<ChannelStrip> outputs;
	KeyBinding keyBinding;
	JackPortAutoConnect jackPortAutoConnect;

	std::unique_ptr<uv_timer_t, void (*)(uv_timer_t*)> fastTimer;
	std::unique_ptr<uv_timer_t, void (*)(uv_timer_t*)> slowTimer;
	bool oscNeedSaveConfig;
	bool audioRunning;

	OscDynamicVariable<std::string> oscTypeList;
	OscDynamicVariable<std::string> oscDeviceList;
#ifdef _WIN32
	OscDynamicVariable<std::string> oscDeviceListWasapi;
#endif
};
