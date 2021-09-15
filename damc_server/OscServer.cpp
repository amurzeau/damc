#include "OscServer.h"
#include <Osc/OscNode.h>
#include <math.h>
#include <spdlog/spdlog.h>
#include <string.h>
#include <tinyosc.h>

OscServer::OscServer(OscRoot* oscRoot, OscContainer* oscParent)
    : OscConnector(oscRoot, false), started(false), oscSendOverUDP(oscParent, "osc_enable_udp", false) {}

OscServer::~OscServer() {}

void OscServer::init(const char* ip, uint16_t port, const char* targetIp, uint16_t targetPort) {
	int ret;

	SPDLOG_INFO("Target OSC UDP address: {}:{}", targetIp, targetPort);

	ret = uv_ip4_addr(targetIp, targetPort, &targetAddress);
	if(ret != 0) {
		SPDLOG_ERROR(
		    "Failed to convert UDP target address {}:{}, error: {} ({})", targetIp, targetPort, uv_strerror(ret), ret);
	}

	loop = uv_default_loop();

	struct sockaddr_in addr;

	SPDLOG_INFO("Listening OSC over UDP on {}:{}", ip, port);
	ret = uv_ip4_addr(ip, port, &addr);
	if(ret != 0) {
		SPDLOG_ERROR("Failed to convert UDP listen address {}:{}: {} ({})", ip, port, uv_strerror(ret), ret);
		return;
	}

	ret = uv_udp_init_ex(loop, &server, AF_INET);
	if(ret != 0) {
		SPDLOG_ERROR("Failed to initialize UDP server: {} ({})", uv_strerror(ret), ret);
		return;
	}

	ret = uv_udp_bind(&server, (const struct sockaddr*) &addr, 0);
	if(ret != 0) {
		SPDLOG_ERROR("Failed to bind UDP server to address {}:{}: {} ({})", ip, port, uv_strerror(ret), ret);
		return;
	}

	server.data = this;
	ret = uv_udp_recv_start(&server, &onAllocData, &onReadData);
	if(ret != 0) {
		SPDLOG_ERROR("Failed to start UDP server", uv_strerror(ret), ret);
		return;
	}

	SPDLOG_INFO("UDP server started");
	started = true;
}

void OscServer::stop() {
	SPDLOG_INFO("Stopping UDP server");
	uv_close((uv_handle_t*) &server, nullptr);
}

void OscServer::sendOscData(const uint8_t* data, size_t size) {
	if(!started || !oscSendOverUDP) {
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
		if(nread == UV_EOF) {
			SPDLOG_INFO("Client disconnected");
		} else {
			SPDLOG_INFO("Error on reading client stream: {}", uv_strerror(nread));
		}
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
