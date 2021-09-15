#include "OscTcpServer.h"
#include "OscTcpClient.h"
#include <math.h>
#include <spdlog/spdlog.h>
#include <string.h>

OscTcpServer::OscTcpServer(OscRoot* oscRoot) : oscRoot(oscRoot) {}

void OscTcpServer::init(const char* ip, uint16_t port) {
	int ret;

	loop = uv_default_loop();

	struct sockaddr_in addr;

	SPDLOG_INFO("Listening OSC over TCP on {}:{}", ip, port);
	ret = uv_ip4_addr(ip, port, &addr);
	if(ret != 0) {
		SPDLOG_ERROR("Failed to convert TCP listen address {}:{}: {} ({})", ip, port, uv_strerror(ret), ret);
		return;
	}

	ret = uv_tcp_init_ex(loop, &server, AF_INET);
	if(ret != 0) {
		SPDLOG_ERROR("Failed to initialize TCP server: {} ({})", uv_strerror(ret), ret);
		return;
	}

	ret = uv_tcp_bind(&server, (const struct sockaddr*) &addr, 0);
	if(ret != 0) {
		SPDLOG_ERROR("Failed to bind TCP server to address {}:{}: {} ({})", ip, port, uv_strerror(ret), ret);
		return;
	}

	server.data = this;
	ret = uv_listen((uv_stream_t*) &server, 10, &onConnection);
	if(ret != 0) {
		SPDLOG_ERROR("Failed to start listening TCP server", uv_strerror(ret), ret);
		return;
	}

	SPDLOG_INFO("TCP server started");
}

void OscTcpServer::stop() {
	SPDLOG_INFO("Stopping TCP server");

	uv_close((uv_handle_t*) &server, nullptr);

	SPDLOG_INFO("Closing {} TCP clients", clients.size());
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
		SPDLOG_ERROR("Error on listening: {} ({})", uv_strerror(status), status);
		return;
	}

	/* dynamically allocate a new client stream object on conn */
	OscTcpClient* client = new OscTcpClient(thisInstance->loop, thisInstance);
	thisInstance->clients.push_back(client);

	/* now let bind the client to the server to be used for incomings */
	int ret = uv_accept(server, (uv_stream_t*) client->getHandle());
	if(ret == 0) {
		client->startRead();
	} else {
		SPDLOG_ERROR("Connection accept error: {} ({})", uv_strerror(ret), ret);
		client->close();
	}
}
