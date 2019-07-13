#ifndef CONTROLSERVER_H
#define CONTROLSERVER_H

#include <list>
#include <stdint.h>

#include <uv.h>

class ControlClient;

class ControlServer {
public:
	typedef void (*PacketCallback)(void* arg, const void* data, size_t size);
	typedef void (*NewClientCallback)(void* arg, ControlClient* client);

public:
	void init(
	    const char* ip, uint16_t port, PacketCallback packetCallback, NewClientCallback newClientCallback, void* arg);
	void run();
	void stop();

	void sendMessage(const void* data, size_t size);

	void removeClient(ControlClient* client);
	void onPacketReceived(const void* data, size_t size) { (*packetCallback)(arg, data, size); }

protected:
	static void onConnection(uv_stream_t* server, int status);

private:
	uint16_t port;
	uv_tcp_t server;
	uv_loop_t* loop;
	std::list<ControlClient*> clients;
	PacketCallback packetCallback;
	NewClientCallback newClientCallback;
	void* arg;
};

#endif
