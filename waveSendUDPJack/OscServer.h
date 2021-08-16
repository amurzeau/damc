#pragma once

#include "OscAddress.h"
#include <list>
#include <memory>
#include <stdint.h>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include <uv.h>

class ControlClient;
class OscNode;
struct tosc_message_const;

class OscServer : public OscContainer {
public:
	typedef void (*PacketCallback)(void* arg, const void* data, size_t size);
	typedef void (*NewClientCallback)(void* arg, ControlClient* client);

public:
	OscServer();
	~OscServer();

	void init(const char* ip, uint16_t port);
	void stop();

	void printAllNodes();

	static void sendMessage(const std::string& address, const OscArgument* argument, size_t number);

	static void triggerAddress(const std::string& address);

	static void addAddress(std::string address, OscNode* oscAddress);
	static void removeAddress(OscNode* oscAddress);

protected:
	void sendNextMessage();

	static void onAllocData(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf);
	static void onReadData(
	    uv_udp_t* handle, ssize_t nread, const uv_buf_t* buf, const struct sockaddr* addr, unsigned flags);
	static void onWrittenData(uv_udp_send_t* req, int status);

	void onPacketReceived(const char* data, size_t size);
	void executeMessage(tosc_message_const* osc);

private:
	uint16_t port;
	uv_udp_t server;
	uv_loop_t* loop;
	void* arg;
	uv_udp_send_t sendReq;
	struct sockaddr_in targetAddress;

	struct MessageView {
		std::vector<uint8_t> buffer;
		size_t sizeToSend;
	};

	bool sending;
	std::list<MessageView> pendingBuffersToSend;
	std::list<MessageView> availableBuffersToSend;

	std::list<std::unique_ptr<char>> availableBuffersForRecv;

	static OscServer* instance;
	std::unordered_map<std::string, OscNode*> variables;
};
