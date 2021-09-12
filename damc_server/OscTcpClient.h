#pragma once

#include <OscRoot.h>
#include <stdint.h>
#include <vector>

#include <uv.h>

class OscTcpServer;

class OscTcpClient : public OscConnector {
public:
	OscTcpClient(uv_loop_t* loop, OscTcpServer* server);

	uv_tcp_t* getHandle() { return &client; }

	void startRead();
	void sendOscData(const uint8_t* buffer, size_t sizeToSend) override;
	void close();

protected:
	static void onAllocData(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf);
	static void onReadData(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf);
	static void onClose(uv_handle_t* handle);
	static void onWriteDone(uv_write_t* req, int status);
	void onDataReceived(const void* data, size_t size);

private:
	OscTcpServer* server;
	uv_tcp_t client;
	std::vector<uint8_t> buffer;
};
