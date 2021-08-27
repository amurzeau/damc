#pragma once

#include <list>
#include <memory>
#include <stdint.h>
#include <string>
#include <unordered_map>
#include <uv.h>
#include <variant>
#include <vector>

#include "OscRoot.h"

class OscNode;
struct tosc_message_const;

class OscServer : public OscConnector {
public:
	OscServer(OscRoot* oscRoot);
	~OscServer();

	void init(const char* ip, uint16_t port);
	void stop();

	static void triggerAddress(const std::string& address);

protected:
	void sendOscData(const uint8_t* data, size_t size) override;

	static void onAllocData(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf);
	static void onReadData(
	    uv_udp_t* handle, ssize_t nread, const uv_buf_t* buf, const struct sockaddr* addr, unsigned flags);
	static void onWriteDone(uv_udp_send_t* req, int status);

private:
	uv_udp_t server;
	uv_loop_t* loop;
	struct sockaddr_in targetAddress;
	bool started;

	struct MessageView {
		std::vector<uint8_t> buffer;
		size_t sizeToSend;
	};

	std::list<std::unique_ptr<char>> availableBuffersForRecv;
};
