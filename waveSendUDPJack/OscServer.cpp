#include "OscServer.h"
#include "ControlClient.h"
#include "OscAddress.h"
#include "tinyosc.h"
#include <math.h>
#include <string.h>

OscServer::OscServer(OscRoot* oscRoot) : OscConnector(oscRoot, false), started(false) {}

OscServer::~OscServer() {}

void OscServer::init(const char* ip, uint16_t port) {
	int ret;

	uv_ip4_addr("127.0.0.1", 10000, &targetAddress);

	loop = uv_default_loop();

	struct sockaddr_in addr;

	ret = uv_ip4_addr(ip, port, &addr);
	if(ret < 0) {
		printf("Can't convert ip address: %s:%u\n", ip, port);
	}

	uv_udp_init_ex(loop, &server, AF_INET);
	uv_udp_bind(&server, (const struct sockaddr*) &addr, 0);

	server.data = this;
	uv_udp_recv_start(&server, &onAllocData, &onReadData);

	started = true;
}

void OscServer::stop() {
	uv_close((uv_handle_t*) &server, nullptr);
}

void OscServer::sendOscData(const uint8_t* data, size_t size) {
	if(!started) {
		return;
	}

	uv_udp_send_t* req = new uv_udp_send_t;
	uv_buf_t buf = uv_buf_init((char*) malloc(size), size);

	memcpy(buf.base, data, size);

	req->data = buf.base;

	uv_udp_send(req, &server, &buf, 1, (const struct sockaddr*) &targetAddress, &onWriteDone);
}

/**
 * Allocates a buffer which we can use for reading.
 */
void OscServer::onAllocData(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
	OscServer* thisInstance = (OscServer*) handle->data;

	if(thisInstance->availableBuffersForRecv.empty()) {
		*buf = uv_buf_init((char*) malloc(65536), 65536);
	} else {
		char* buffer = thisInstance->availableBuffersForRecv.front().release();
		thisInstance->availableBuffersForRecv.pop_front();
		*buf = uv_buf_init(buffer, 65536);
	}
}

/**
 * Callback which is executed on each readable state.
 */
void OscServer::onReadData(
    uv_udp_t* handle, ssize_t nread, const uv_buf_t* buf, const struct sockaddr* addr, unsigned flags) {
	OscServer* thisInstance = (OscServer*) handle->data;

	/* if read bytes counter -1 there is an error or EOF */
	if(nread < 0) {
		if(nread != UV_EOF)
			fprintf(stderr, "Error on reading client stream: %s.\n", uv_strerror(nread));
	} else if(nread > 0) {
		thisInstance->onOscDataReceived((const uint8_t*) buf->base, nread);
	}

	/* free the remaining memory */
	thisInstance->availableBuffersForRecv.emplace_back(buf->base);
}

void OscServer::onWriteDone(uv_udp_send_t* req, int status) {
	free(req->data);
	delete req;
}
