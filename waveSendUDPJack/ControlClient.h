#ifndef CONTROLCLIENT_H
#define CONTROLCLIENT_H

#include <stdint.h>
#include <vector>

#include <uv.h>

class ControlServer;

class ControlClient {
public:
	ControlClient(uv_loop_t* loop, ControlServer* server);

	uv_tcp_t* getHandle() { return &client; }

	void startRead();
	void sendMessage(const void* data, size_t size);
	void close();

protected:
	static void onAllocData(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf);
	static void onReadData(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf);
	static void onClose(uv_handle_t* handle);
	static void onWriteDone(uv_write_t* req, int status);
	void onDataReceived(const void* data, size_t size);

private:
	ControlServer* server;
	uv_tcp_t client;
	std::vector<uint8_t> buffer;
};

#endif
