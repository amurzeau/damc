#pragma once

#include <OscRoot.h>
#include <list>
#include <stdint.h>

#include <uv.h>

class OscTcpClient;

class OscTcpServer {
public:
	OscTcpServer(OscRoot* oscRoot);

public:
	void init(const char* ip, uint16_t port);
	void run();
	void stop();

	void sendMessage(const void* data, size_t size);

	void removeClient(OscTcpClient* client);

	OscRoot* getOscRoot() { return oscRoot; }

protected:
	static void onConnection(uv_stream_t* server, int status);

private:
	OscRoot* oscRoot;
	uint16_t port;
	uv_tcp_t server;
	uv_loop_t* loop;
	std::list<OscTcpClient*> clients;
};
