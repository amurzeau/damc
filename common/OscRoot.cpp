#include "OscRoot.h"
#include "OscAddress.h"
#include "tinyosc.h"
#include <math.h>
#include <string.h>

OscRoot::OscRoot() : OscContainer(nullptr, "") {
	sending = false;
}

OscRoot::~OscRoot() {}

void OscRoot::printAllNodes() {
	printf("Nodes:\n%s\n", getAsString().c_str());
}

void OscRoot::processNextMessage() {
	if(sending || pendingBuffersToSend.empty()) {
		return;
	}

	const auto& buffer = pendingBuffersToSend.front();

	sending = true;
	sendNextMessage(&buffer.buffer[0], buffer.sizeToSend);
}

void OscRoot::sendMessage(const std::string& address, const OscArgument* arguments, size_t number) {
	tosc_message osc;
	MessageView sendBuffer;
	char format[128] = ",";
	char* formatPtr = format + 1;

	if(number > sizeof(format) - 2) {
		printf("ERROR: Too many arguments: %d\n", number);
		return;
	}

	// reuse allocated memory
	if(!availableBuffersToSend.empty()) {
		sendBuffer.buffer.swap(availableBuffersToSend.front().buffer);
		availableBuffersToSend.pop_front();
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
	pendingBuffersToSend.push_back(std::move(sendBuffer));

	processNextMessage();
}

void OscRoot::onMessageSent() {
	availableBuffersToSend.push_back(std::move(pendingBuffersToSend.front()));
	pendingBuffersToSend.pop_front();

	sending = false;
	processNextMessage();
}

void OscRoot::onPacketReceived(const char* data, size_t size) {
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

void OscRoot::executeMessage(tosc_message_const* osc) {
	std::vector<OscArgument> arguments;
	const char* address = tosc_getAddress(osc);

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

	execute(address + 1, std::move(arguments));
}

void OscRoot::triggerAddress(const std::string& address) {
	execute(address.c_str() + 1, std::vector<OscArgument>{});
}
