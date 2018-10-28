#include "ControlClient.h"
#include "ControlServer.h"

#include <math.h>
#include <string.h>

ControlClient::ControlClient(uv_loop_t* loop, ControlServer* server) : server(server) {
	uv_tcp_init(loop, &client);
	client.data = this;
}

void ControlClient::startRead() {
	/* start reading from stream */
	int r = uv_read_start((uv_stream_t*) &client, &onAllocData, &onReadData);
	if(r) {
		fprintf(stderr, "Error on reading client stream: %s.\n", uv_strerror(r));
		close();
	}
}

void ControlClient::sendMessage(const void* data, size_t size) {
	uv_write_t* req = new uv_write_t;
	uv_buf_t buf = uv_buf_init((char*) malloc(size + sizeof(int)), size + sizeof(int));

	*reinterpret_cast<int*>(buf.base) = size;
	memcpy(buf.base + sizeof(int), (char*) data, size);

	req->data = buf.base;

	uv_write(req, (uv_stream_t*) &client, &buf, 1, &onWriteDone);
}

void ControlClient::close() {
	uv_close((uv_handle_t*) &client, &onClose);
}

/**
 * Callback which is executed on each readable state.
 */
void ControlClient::onReadData(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf) {
	ControlClient* thisInstance = (ControlClient*) stream->data;

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
void ControlClient::onAllocData(uv_handle_t*, size_t suggested_size, uv_buf_t* buf) {
	*buf = uv_buf_init((char*) malloc(suggested_size), suggested_size);
}

/**
 * Allocates a buffer which we can use for reading.
 */
void ControlClient::onClose(uv_handle_t* handle) {
	ControlClient* thisInstance = (ControlClient*) handle->data;
	thisInstance->server->removeClient(thisInstance);
	delete thisInstance;
}

void ControlClient::onWriteDone(uv_write_t* req, int) {
	free(req->data);
	delete req;
}

void ControlClient::onDataReceived(const void* data, size_t size) {
	buffer.insert(buffer.end(), (uint8_t*) data, (uint8_t*) data + size);

	printf("Received %d data, buffer contains %d bytes\n", (int) size, (int) buffer.size());

	do {
		size_t currentMessageSize;

		if(buffer.size() >= sizeof(int)) {
			currentMessageSize = *reinterpret_cast<int*>(buffer.data());
		} else {
			// not enough data to get size
			break;
		}

		printf("Current message size = %d\n", (int) currentMessageSize);

		// not enough data to have full packet
		if(buffer.size() < currentMessageSize + sizeof(int))
			break;

		server->onPacketReceived(buffer.data() + sizeof(int), currentMessageSize);
		buffer.erase(buffer.begin(), buffer.begin() + currentMessageSize + sizeof(int));

	} while(!buffer.empty());
}
