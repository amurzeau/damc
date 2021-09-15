#include "KeyBinding.h"
#include "OscRoot.h"
#include <functional>
#include <spdlog/spdlog.h>

#ifdef _WIN32
#include <Windows.h>
#endif

uint32_t KeyBinding::next_hotkey_id;

#define WM_HOTKEY_ADD WM_USER
#define WM_HOTKEY_REMOVE (WM_USER + 1)

struct MessageHotkeyControlData {
	uint32_t modifiers;
	uint32_t virtualKeyCode;
	std::string targetOscAddress;
};

KeyBinding::KeyBinding(OscRoot* oscRoot, OscContainer* parent)
    : oscRoot(oscRoot), oscAddShortcutEnpoint(parent, "add"), oscRemoveShortcutEnpoint(parent, "remove") {
	SPDLOG_INFO("Initializing hotkeys listener");

	uv_async_init(uv_default_loop(), &oscTriggerAsync, &KeyBinding::onOscTriggered);
	oscTriggerAsync.data = this;
	uv_unref((uv_handle_t*) &oscTriggerAsync);

	oscAddShortcutEnpoint.setCallback([this](auto arg) { oscAddShortcut(arg); });
	oscRemoveShortcutEnpoint.setCallback([this](auto arg) { oscRemoveShortcut(arg); });

#ifdef _WIN32
	SPDLOG_INFO("Starting hotkeys listener");
	hotkeyListenerThread = std::make_unique<std::thread>(&KeyBinding::threadHandleShortcuts, this);
#endif
}

KeyBinding::~KeyBinding() {
#ifdef _WIN32
	PostThreadMessage(hotkeyListenerThreadId, WM_QUIT, 0, 0);
	hotkeyListenerThread->join();
#endif
}

void KeyBinding::oscAddShortcut(std::vector<OscArgument> args) {
#ifdef _WIN32
	if(!check_osc_arguments<int32_t, int32_t, std::string>(args))
		return;

	MessageHotkeyControlData* message = new MessageHotkeyControlData;

	message->virtualKeyCode = std::get<int32_t>(args[0]);
	message->modifiers = std::get<int32_t>(args[1]);
	message->targetOscAddress = std::get<std::string>(args[2]);

	if(PostThreadMessage(hotkeyListenerThreadId, WM_HOTKEY_ADD, 0, (LPARAM) message) == FALSE) {
		delete message;
	}
#endif
}

void KeyBinding::oscRemoveShortcut(std::vector<OscArgument> args) {
#ifdef _WIN32
	if(!check_osc_arguments<int32_t, int32_t, std::string>(args))
		return;

	MessageHotkeyControlData* message = new MessageHotkeyControlData;

	message->virtualKeyCode = std::get<int32_t>(args[0]);
	message->modifiers = std::get<int32_t>(args[1]);
	message->targetOscAddress = std::get<std::string>(args[2]);

	if(PostThreadMessage(hotkeyListenerThreadId, WM_HOTKEY_REMOVE, 0, (LPARAM) message) == FALSE) {
		delete message;
	}
#endif
}

void KeyBinding::threadHandleShortcuts() {
#ifdef _WIN32
	std::unordered_map<Hotkey, HotkeyData, HotkeyHasher> registeredKeys;

	hotkeyListenerThreadId = GetCurrentThreadId();

	SPDLOG_INFO("Started hotkeys listener with {} hotkeys", registeredKeys.size());

	MSG msg;
	while(GetMessage(&msg, nullptr, 0, 0) != 0) {
		switch(msg.message) {
			case WM_HOTKEY: {
				Hotkey hotkey{HIWORD(msg.lParam), LOWORD(msg.lParam)};

				SPDLOG_INFO(
				    "Received hotkey for modified {} and key virtual code {}", hotkey.modifiers, hotkey.virtualKeyCode);

				auto it = registeredKeys.find(hotkey);

				if(it == registeredKeys.end()) {
					SPDLOG_WARN("Hotkey {}:{} not found, ignoring", hotkey.modifiers, hotkey.virtualKeyCode);
					break;
				}

				{
					std::lock_guard lock(pendingTriggeredOscAddressesMutex);

					for(const std::string& address : it->second.oscAddress) {
						SPDLOG_DEBUG("Adding pending OSC address {}", address);
						pendingTriggeredOscAddresses.push_back(address);
					}
					uv_async_send(&oscTriggerAsync);
				}
				break;
			}

			case WM_HOTKEY_ADD: {
				std::unique_ptr<MessageHotkeyControlData> message{(MessageHotkeyControlData*) msg.lParam};
				Hotkey hotkey{message->virtualKeyCode, message->modifiers};

				auto it = registeredKeys.find(hotkey);

				if(it == registeredKeys.end()) {
					SPDLOG_INFO(
					    "Added new hotkey {}:{} with id {}", hotkey.modifiers, hotkey.virtualKeyCode, next_hotkey_id);
					RegisterHotKey(nullptr, next_hotkey_id, hotkey.modifiers, hotkey.virtualKeyCode);
					registeredKeys[hotkey].id = next_hotkey_id;
					next_hotkey_id++;
				}

				if(registeredKeys[hotkey].oscAddress.insert(message->targetOscAddress).second) {
					SPDLOG_INFO("Hotkey {}:{} added with OSC target {}",
					            hotkey.modifiers,
					            hotkey.virtualKeyCode,
					            message->targetOscAddress);
				} else {
					SPDLOG_INFO("Hotkey {}:{} already has target {}",
					            hotkey.modifiers,
					            hotkey.virtualKeyCode,
					            message->targetOscAddress);
				}
				break;
			}

			case WM_HOTKEY_REMOVE: {
				std::unique_ptr<MessageHotkeyControlData> message{(MessageHotkeyControlData*) msg.lParam};
				Hotkey hotkey{message->virtualKeyCode, message->modifiers};

				auto it = registeredKeys.find(hotkey);

				if(it == registeredKeys.end()) {
					SPDLOG_WARN("Hotkey {}:{} not found, cannot remove it", hotkey.modifiers, hotkey.virtualKeyCode);
					return;
				}

				if(it->second.oscAddress.erase(message->targetOscAddress) > 0) {
					SPDLOG_INFO("Hotkey {}:{} removed target {}",
					            hotkey.modifiers,
					            hotkey.virtualKeyCode,
					            message->targetOscAddress);
				} else {
					SPDLOG_WARN("Hotkey {}:{} is not associated with {}",
					            hotkey.modifiers,
					            hotkey.virtualKeyCode,
					            message->targetOscAddress);
				}

				if(it->second.oscAddress.empty()) {
					SPDLOG_DEBUG("No more associated hotkey with {}:{}, removing id {}",
					             hotkey.modifiers,
					             hotkey.virtualKeyCode,
					             it->second.id);
					UnregisterHotKey(nullptr, it->second.id);
					registeredKeys.erase(it);
				}
				break;
			}
		}
	}
#endif
}

void KeyBinding::onOscTriggered(uv_async_t* handle) {
	KeyBinding* instance = (KeyBinding*) handle->data;
	std::vector<std::string> oscAddressesToTrigger;

	{
		std::lock_guard lock(instance->pendingTriggeredOscAddressesMutex);
		oscAddressesToTrigger.swap(instance->pendingTriggeredOscAddresses);
	}

	for(const std::string& address : oscAddressesToTrigger) {
		SPDLOG_INFO("Executing OSC address {}", address);
		instance->oscRoot->triggerAddress(address);
	}
}
