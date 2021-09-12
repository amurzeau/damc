#include "OscTcpServer.h"
#include "OscTcpClient.h"
#include <math.h>
#include <string.h>

OscTcpServer::OscTcpServer(OscRoot* oscRoot) : oscRoot(oscRoot) {}

void OscTcpServer::init(const char* ip, uint16_t port) {
	int ret;

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

void OscTcpServer::run() {
	uv_run(loop, UV_RUN_DEFAULT);
}

void OscTcpServer::stop() {
	uv_close((uv_handle_t*) &server, nullptr);
	for(OscTcpClient* client : clients) {
		client->close();
	}
}

void OscTcpServer::removeClient(OscTcpClient* client) {
	clients.remove(client);
}

void OscTcpServer::onConnection(uv_stream_t* server, int status) {
	OscTcpServer* thisInstance = (OscTcpServer*) server->data;

	/* if status not zero there was an error */
	if(status < 0) {
		fprintf(stderr, "Error on listening: %s.\n", uv_strerror(status));
		return;
	}

	/* dynamically allocate a new client stream object on conn */
	OscTcpClient* client = new OscTcpClient(thisInstance->loop, thisInstance);
	thisInstance->clients.push_back(client);

	/* now let bind the client to the server to be used for incomings */
	if(uv_accept(server, (uv_stream_t*) client->getHandle()) == 0) {
		client->startRead();
	} else {
		client->close();
	}
}
