#pragma once

#include "OscAddress.h"
#include <list>
#include <memory>
#include <stdint.h>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

class ControlClient;
class OscNode;
struct tosc_message_const;

class OscRoot : public OscContainer {
public:
	OscRoot();
	~OscRoot();

	void onMessageSent();

	void printAllNodes();

	void triggerAddress(const std::string& address);

protected:
	void processNextMessage();
	void onPacketReceived(const char* data, size_t size);
	void executeMessage(tosc_message_const* osc);
	void sendMessage(const std::string& address, const OscArgument* argument, size_t number) override;

	virtual void sendNextMessage(const uint8_t* data, size_t size) = 0;

private:
	struct MessageView {
		std::vector<uint8_t> buffer;
		size_t sizeToSend;
	};

	bool sending;
	std::list<MessageView> pendingBuffersToSend;
	std::list<MessageView> availableBuffersToSend;
};
