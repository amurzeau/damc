#include "ControlServer.h"
#include "ControlClient.h"
#include <math.h>
#include <string.h>

void ControlServer::init(
    const char* ip, uint16_t port, PacketCallback packetCallback, NewClientCallback newClientCallback, void* arg) {
	int ret;

	this->packetCallback = packetCallback;
	this->newClientCallback = newClientCallback;
	this->arg = arg;

	loop = uv_default_loop();

	struct sockaddr_in addr;

	ret = uv_ip4_addr(ip, port, &addr);
	if(ret < 0) {
		printf("Can't convert ip address: %s:%u\n", ip, port);
	}

	uv_tcp_init_ex(loop, &server, AF_INET);
	uv_tcp_bind(&server, (const struct sockaddr*) &addr, 0);

	server.data = this;
	uv_listen((uv_stream_t*) &server, 10, &onConnection);
}

void ControlServer::run() {
	uv_run(loop, UV_RUN_DEFAULT);
}

void ControlServer::sendMessage(const void* data, size_t size) {
	for(ControlClient* client : clients) {
		client->sendMessage(data, size);
	}
}

void ControlServer::removeClient(ControlClient* client) {
	clients.remove(client);
}

void ControlServer::onConnection(uv_stream_t* server, int status) {
	ControlServer* thisInstance = (ControlServer*) server->data;

	/* if status not zero there was an error */
	if(status < 0) {
		fprintf(stderr, "Error on listening: %s.\n", uv_strerror(status));
		return;
	}

	/* dynamically allocate a new client stream object on conn */
	ControlClient* client = new ControlClient(thisInstance->loop, thisInstance);
	thisInstance->clients.push_back(client);

	/* now let bind the client to the server to be used for incomings */
	if(uv_accept(server, (uv_stream_t*) client->getHandle()) == 0) {
		client->startRead();
		(*thisInstance->newClientCallback)(thisInstance->arg, client);
	} else {
		client->close();
	}
}
