#pragma once

#include <Osc/OscEndpoint.h>
#include <atomic>
#include <memory>
#include <mutex>
#include <set>
#include <stdint.h>
#include <string>
#include <thread>
#include <uv.h>

class OscRoot;

class KeyBinding {
public:
	struct Hotkey {
		Hotkey(uint32_t virtualKeyCode, uint32_t modifiers) : virtualKeyCode(virtualKeyCode), modifiers(modifiers) {}
		bool operator==(const Hotkey& other) const {
			return other.virtualKeyCode == virtualKeyCode && other.modifiers == modifiers;
		}

		const uint32_t virtualKeyCode;
		const uint32_t modifiers;
	};

	KeyBinding(OscRoot* oscRoot, OscContainer* parent);
	~KeyBinding();

protected:
	void oscAddShortcut(std::vector<OscArgument> args);
	void oscRemoveShortcut(std::vector<OscArgument> args);

	void threadHandleShortcuts();
	static void onOscTriggered(uv_async_t* handle);

private:
	struct HotkeyHasher {
		size_t operator()(const Hotkey& k) const {
			return (size_t)((((uint64_t) 12638153115695167455u ^ k.virtualKeyCode) * 1099511628211 ^ k.modifiers));
		}
	};

	struct HotkeyData {
		std::set<std::string> oscAddress;
		uint16_t id;
	};

	OscRoot* oscRoot;
	OscEndpoint oscAddShortcutEnpoint;
	OscEndpoint oscRemoveShortcutEnpoint;

#ifdef _WIN32
	std::unique_ptr<std::thread> hotkeyListenerThread;
	std::atomic<uint32_t> hotkeyListenerThreadId;
#endif

	uv_async_t oscTriggerAsync;
	std::mutex pendingTriggeredOscAddressesMutex;
	std::vector<std::string> pendingTriggeredOscAddresses;

	static uint32_t next_hotkey_id;
};
