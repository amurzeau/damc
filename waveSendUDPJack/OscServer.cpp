#include "OscServer.h"
#include "ControlClient.h"
#include "OscAddress.h"
#include "tinyosc.h"
#include <math.h>
#include <string.h>

OscServer* OscServer::instance;

OscServer::OscServer() : OscContainer(nullptr, "") {
	instance = this;
	sending = false;
}

OscServer::~OscServer() {
	instance = nullptr;
}

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
}

void OscServer::stop() {
	uv_close((uv_handle_t*) &server, nullptr);
}

void OscServer::printAllNodes() {
	printf("Nodes:\n%s\n", getAsString().c_str());
}

void OscServer::sendNextMessage() {
	if(sending || pendingBuffersToSend.empty()) {
		return;
	}

	const auto& buffer = pendingBuffersToSend.front();

	uv_buf_t buf = uv_buf_init((char*) &buffer.buffer[0], buffer.sizeToSend);
	sendReq.data = this;
	sending = true;
	uv_udp_send(&sendReq, &server, &buf, 1, (const struct sockaddr*) &targetAddress, &onWrittenData);
}

void OscServer::sendMessage(const std::string& address, const OscArgument* arguments, size_t number) {
	if(!instance)
		return;

	tosc_message osc;
	MessageView sendBuffer;
	char format[128] = ",";
	char* formatPtr = format + 1;

	if(number > sizeof(format) - 2) {
		printf("ERROR: Too many arguments: %d\n", number);
		return;
	}

	// reuse allocated memory
	if(!instance->availableBuffersToSend.empty()) {
		sendBuffer.buffer.swap(instance->availableBuffersToSend.front().buffer);
		instance->availableBuffersToSend.pop_front();
	}
	if(sendBuffer.buffer.size() == 0) {
		sendBuffer.buffer.resize(1024);
	}

	for(size_t i = 0; i < number; i++) {
		const OscArgument& argument = arguments[i];
		std::visit(
		    [&formatPtr](auto&& arg) -> void {
			    using U = std::decay_t<decltype(arg)>;
			    if constexpr(std::is_same_v<U, bool>) {
				    *formatPtr++ = arg ? 'T' : 'F';
			    } else if constexpr(std::is_same_v<U, int32_t>) {
				    *formatPtr++ = 'i';
			    } else if constexpr(std::is_same_v<U, float>) {
				    *formatPtr++ = 'f';
			    } else if constexpr(std::is_same_v<U, std::string>) {
				    *formatPtr++ = 's';
			    } else {
				    static_assert(always_false_v<U>, "Unhandled type");
			    }
		    },
		    argument);
	}
	*formatPtr++ = '\0';

	if(tosc_writeMessageHeader(
	       &osc, address.c_str(), format, (char*) &sendBuffer.buffer[0], sendBuffer.buffer.size()) != 0) {
		printf("failed to write message\n");
		return;
	}

	for(size_t i = 0; i < number; i++) {
		const OscArgument& argument = arguments[i];
		uint32_t result = 0;
		std::visit(
		    [&osc, &result](auto&& arg) -> void {
			    using U = std::decay_t<decltype(arg)>;
			    if constexpr(std::is_same_v<U, bool>) {
				    // No argument to write
			    } else if constexpr(std::is_same_v<U, int32_t>) {
				    result = tosc_writeNextInt32(&osc, arg);
			    } else if constexpr(std::is_same_v<U, float>) {
				    result = tosc_writeNextFloat(&osc, arg);
			    } else if constexpr(std::is_same_v<U, std::string>) {
				    result = tosc_writeNextString(&osc, arg.c_str());
			    } else {
				    static_assert(always_false_v<U>, "Unhandled type");
			    }
		    },
		    argument);

		if(result != 0) {
			printf("failed to write message\n");
			return;
		}
	}

	sendBuffer.sizeToSend = tosc_getMessageLength(&osc);
	instance->pendingBuffersToSend.push_back(std::move(sendBuffer));

	instance->sendNextMessage();
}

void OscServer::addAddress(std::string address, OscNode* oscAddress) {
	if(!instance)
		return;

	instance->variables.emplace(address, oscAddress);
	// printf("Adding address %s\n", address.c_str());
}

void OscServer::removeAddress(OscNode* oscAddress) {
	if(!instance)
		return;

	for(auto it = instance->variables.begin(); it != instance->variables.end();) {
		if(it->second == oscAddress) {
			printf("Removing address %s\n", it->first.c_str());
			instance->variables.erase(it++);
		} else {
			++it;
		}
	}
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
		thisInstance->onPacketReceived(buf->base, nread);
	}

	/* free the remaining memory */
	thisInstance->availableBuffersForRecv.emplace_back(buf->base);
}

void OscServer::onWrittenData(uv_udp_send_t* req, int status) {
	OscServer* thisInstance = (OscServer*) req->data;

	thisInstance->availableBuffersToSend.push_back(std::move(thisInstance->pendingBuffersToSend.front()));
	thisInstance->pendingBuffersToSend.pop_front();

	thisInstance->sending = false;
	thisInstance->sendNextMessage();
}

void OscServer::onPacketReceived(const char* data, size_t size) {
	if(tosc_isBundle(data)) {
		tosc_bundle_const bundle;
		tosc_parseBundle(&bundle, data, size);

		tosc_message_const osc;
		while(tosc_getNextMessage(&bundle, &osc)) {
			tosc_printMessage(&osc);
			executeMessage(&osc);
		}
	} else {
		tosc_printOscBuffer(data, size);

		tosc_message_const osc;
		int result = tosc_parseMessage(&osc, data, size);
		if(result == 0) {
			executeMessage(&osc);
		}
	}
}

void OscServer::executeMessage(tosc_message_const* osc) {
	std::vector<OscArgument> arguments;
	const char* address = tosc_getAddress(osc);

	//	auto it = variables.find(std::string(address));
	//	if(it == variables.end()) {
	//		printf("Address %s not recognized\n", address);
	//		return;
	//	}

	//	OscNode* oscAddress = it->second;

	for(int i = 0; osc->format[i] != '\0'; i++) {
		OscArgument argument;

		switch(osc->format[i]) {
			case 's':
				argument = std::string(tosc_getNextString(osc));
				break;
			case 'f':
				argument = tosc_getNextFloat(osc);
				break;
			case 'i':
				argument = tosc_getNextInt32(osc);
				break;
			case 'F':
				argument = false;
				break;
			case 'I':
				argument = INFINITY;
				break;
			case 'T':
				argument = true;
				break;

				// Unsupported formats
			case 'N':
				printf(" Unsupported format: '%c'", osc->format[i]);
				break;
			case 'b': {
				const char* b = nullptr;  // will point to binary data
				int n = 0;                // takes the length of the blob
				tosc_getNextBlob(osc, &b, &n);
				printf(" Unsupported format: '%c'", osc->format[i]);
				break;
			}
			case 'm':
				tosc_getNextMidi(osc);
				printf(" Unsupported format: '%c'", osc->format[i]);
				break;
			case 'd':
				tosc_getNextDouble(osc);
				printf(" Unsupported format: '%c'", osc->format[i]);
				break;
			case 'h':
				tosc_getNextInt64(osc);
				printf(" Unsupported format: '%c'", osc->format[i]);
				break;
			case 't':
				tosc_getNextTimetag(osc);
				printf(" Unsupported format: '%c'", osc->format[i]);
				break;

			default:
				printf(" Unsupported format: '%c'", osc->format[i]);
				break;
		}

		arguments.push_back(std::move(argument));
	}

	// oscAddress->execute(std::move(arguments));
	execute(address + 1, std::move(arguments));
}

void OscServer::triggerAddress(const std::string& address) {
	if(!instance)
		return;

	instance->execute(address.c_str() + 1, std::vector<OscArgument>{});

	//	auto it = instance->variables.find(address);
	//	if(it == instance->variables.end()) {
	//		printf("Address %s not recognized\n", address.c_str());
	//		return;
	//	}

	//	OscNode* oscAddress = it->second;
	//	oscAddress->execute(std::vector<OscArgument>{});
}
