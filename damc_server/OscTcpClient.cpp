#include "OscTcpClient.h"
#include "OscTcpServer.h"

#include <math.h>
#include <string.h>

OscTcpClient::OscTcpClient(uv_loop_t* loop, OscTcpServer* server)
    : OscConnector(server->getOscRoot(), true), server(server) {
	uv_tcp_init(loop, &client);
	client.data = this;
}

void OscTcpClient::startRead() {
	/* start reading from stream */
	int r = uv_read_start((uv_stream_t*) &client, &onAllocData, &onReadData);
	if(r) {
		fprintf(stderr, "Error on reading client stream: %s.\n", uv_strerror(r));
		close();
	}
}

void OscTcpClient::sendOscData(const uint8_t* data, size_t size) {
	uv_write_t* req = new uv_write_t;
	uv_buf_t buf = uv_buf_init((char*) malloc(size), size);

	memcpy(buf.base, data, size);

	req->data = buf.base;

	uv_write(req, (uv_stream_t*) &client, &buf, 1, &onWriteDone);
}

void OscTcpClient::close() {
	uv_close((uv_handle_t*) &client, &onClose);
}

/**
 * Callback which is executed on each readable state.
 */
void OscTcpClient::onReadData(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf) {
	OscTcpClient* thisInstance = (OscTcpClient*) stream->data;

	/* if read bytes counter -1 there is an error or EOF */
	if(nread < 0) {
		if(nread != UV_EOF)
			fprintf(stderr, "Error on reading client stream: %s.\n", uv_strerror(nread));

		thisInstance->close();
	} else {
		thisInstance->onDataReceived(buf->base, nread);
	}

	/* free the remaining memory */
	free(buf->base);
}

/**
 * Allocates a buffer which we can use for reading.
 */
void OscTcpClient::onAllocData(uv_handle_t*, size_t suggested_size, uv_buf_t* buf) {
	*buf = uv_buf_init((char*) malloc(suggested_size), suggested_size);
}

/**
 * Allocates a buffer which we can use for reading.
 */
void OscTcpClient::onClose(uv_handle_t* handle) {
	OscTcpClient* thisInstance = (OscTcpClient*) handle->data;
	thisInstance->server->removeClient(thisInstance);
	delete thisInstance;
}

void OscTcpClient::onWriteDone(uv_write_t* req, int) {
	free(req->data);
	delete req;
}

void OscTcpClient::onDataReceived(const void* data, size_t size) {
	onOscDataReceived((const uint8_t*) data, size);
}
