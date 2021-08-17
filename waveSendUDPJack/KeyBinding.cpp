#include "KeyBinding.h"
#include "OscServer.h"
#include <functional>

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

KeyBinding::KeyBinding(OscContainer* parent)
    : oscAddShortcutEnpoint(parent, "add"), oscRemoveShortcutEnpoint(parent, "remove") {
	uv_async_init(uv_default_loop(), &oscTriggerAsync, &KeyBinding::onOscTriggered);
	oscTriggerAsync.data = this;
	uv_unref((uv_handle_t*) &oscTriggerAsync);

	oscAddShortcutEnpoint.setCallback([this](auto arg) { oscAddShortcut(arg); });
	oscRemoveShortcutEnpoint.setCallback([this](auto arg) { oscRemoveShortcut(arg); });

#ifdef _WIN32
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

	MSG msg;
	while(GetMessage(&msg, nullptr, 0, 0) != 0) {
		switch(msg.message) {
			case WM_HOTKEY: {
				Hotkey hotkey{HIWORD(msg.lParam), LOWORD(msg.lParam)};

				printf("Received hotkey %d:%d\n", hotkey.modifiers, hotkey.virtualKeyCode);

				auto it = registeredKeys.find(hotkey);

				if(it == registeredKeys.end()) {
					printf("Hotkey %d:%d not found\n", hotkey.modifiers, hotkey.virtualKeyCode);
					break;
				}

				{
					std::lock_guard lock(pendingTriggeredOscAddressesMutex);

					for(const std::string& address : it->second.oscAddress) {
						printf("Adding pending OSC address %s\n", address.c_str());
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
					printf(
					    "Added new hotkey %d:%d with id %d\n", hotkey.modifiers, hotkey.virtualKeyCode, next_hotkey_id);
					RegisterHotKey(nullptr, next_hotkey_id, hotkey.modifiers, hotkey.virtualKeyCode);
					registeredKeys[hotkey].id = next_hotkey_id;
					next_hotkey_id++;
				}

				if(registeredKeys[hotkey].oscAddress.insert(message->targetOscAddress).second) {
					printf("Hotkey %d:%d added target %s\n",
					       hotkey.modifiers,
					       hotkey.virtualKeyCode,
					       message->targetOscAddress.c_str());
				} else {
					printf("Hotkey %d:%d already has target %s\n",
					       hotkey.modifiers,
					       hotkey.virtualKeyCode,
					       message->targetOscAddress.c_str());
				}
				break;
			}

			case WM_HOTKEY_REMOVE: {
				std::unique_ptr<MessageHotkeyControlData> message{(MessageHotkeyControlData*) msg.lParam};
				Hotkey hotkey{message->virtualKeyCode, message->modifiers};

				auto it = registeredKeys.find(hotkey);

				if(it == registeredKeys.end()) {
					printf("Hotkey %d:%d not found\n", hotkey.modifiers, hotkey.virtualKeyCode);
					return;
				}

				if(it->second.oscAddress.erase(message->targetOscAddress) > 0) {
					printf("Hotkey %d:%d removed target %s\n",
					       hotkey.modifiers,
					       hotkey.virtualKeyCode,
					       message->targetOscAddress.c_str());
				} else {
					printf("Hotkey %d:%d is not associated with %s\n",
					       hotkey.modifiers,
					       hotkey.virtualKeyCode,
					       message->targetOscAddress.c_str());
				}

				if(it->second.oscAddress.empty()) {
					printf("No more associated hotkey with %d:%d, removing id %d\n",
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
		printf("Executing OSC address %s\n", address.c_str());
		OscServer::triggerAddress(address);
	}
}
