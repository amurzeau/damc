#include "RemoteUdpOutput.h"
#include "gui/MessageInterface.h"
#include <arpa/inet.h>
#include <fcntl.h>
#include <string.h>

static long VBAN_SRList[] = {6000,   12000,  24000,  48000, 96000, 192000, 384000, 8000,   16000,  32000, 64000,
                             128000, 256000, 512000, 11025, 22050, 44100,  88200,  176400, 352800, 705600};
#define VBAN_PROTOCOL_AUDIO 0x00
#define VBAN_PROTOCOL_SERIAL 0x20
#define VBAN_PROTOCOL_TXT 0x40
#define VBAN_PROTOCOL_SERVICE 0x60

#define VBAN_DATATYPE_BYTE8 0x00
#define VBAN_DATATYPE_INT16 0x01
#define VBAN_DATATYPE_INT24 0x02
#define VBAN_DATATYPE_INT32 0x03
#define VBAN_DATATYPE_FLOAT32 0x04
#define VBAN_DATATYPE_FLOAT64 0x05
#define VBAN_DATATYPE_12BITS 0x06
#define VBAN_DATATYPE_10BITS 0x0

#define VBAN_CODEC_PCM 0x00

RemoteUdpOutput::RemoteUdpOutput() {}

RemoteUdpOutput::~RemoteUdpOutput() {}

int RemoteUdpOutput::init(int index, int samplerate, const char* ip, int port) {
	std::string streamName = "waveSendUDP-" + std::to_string(index);
	printf("Sending audio %d to %s:%d\n", index, ip, port);

	sin_server.sin_addr.s_addr = inet_addr(ip);
	sin_server.sin_family = AF_INET;
	sin_server.sin_port = htons(port);

	if((sock_fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		printf("Erreur Winsock");
		return 2;
	}

	int flags;
	flags = fcntl(sock_fd, F_GETFL, 0);
	fcntl(sock_fd, F_SETFL, flags | O_NONBLOCK);
	flags = 1;
	setsockopt(sock_fd, SOL_SOCKET, SO_BROADCAST, &flags, sizeof(flags));

	dataBuffer.header.vban = 0x4e414256;  // "VBAN"
	for(size_t i = 0; i < sizeof(VBAN_SRList) / sizeof(VBAN_SRList[0]); i++) {
		if(VBAN_SRList[i] == samplerate) {
			dataBuffer.header.format_SR = i;
			break;
		}
	}
	dataBuffer.header.format_nbc = 2 - 1;                // numChannels - 1 channel
	dataBuffer.header.format_bit = VBAN_DATATYPE_INT16;  // INT16 PCM
	strncpy(dataBuffer.header.streamname, streamName.c_str(), sizeof(dataBuffer.header.streamname));
	dataBuffer.header.nuFrame = 0;

	return 0;
}

void RemoteUdpOutput::sendAudio(int16_t* samplesLeft, int16_t* samplesRight, size_t samplesCount) {
	for(size_t samplesSent = 0; samplesSent < samplesCount;) {
		size_t samplesToSend = samplesCount - samplesSent;
		if(samplesToSend > 256)
			samplesToSend = 256;

		dataBuffer.header.format_nbs = samplesToSend - 1;

		for(size_t i = 0; i < samplesToSend; i++) {
			dataBuffer.data[i * 2] = samplesLeft[i + samplesSent];
			dataBuffer.data[i * 2 + 1] = samplesRight[i + samplesSent];
		}

		int ret = sendto(sock_fd,
		                 (char*) &dataBuffer,
		                 sizeof(dataBuffer.header) + samplesToSend * 2 * sizeof(dataBuffer.data[0]),
		                 MSG_NOSIGNAL,
		                 (const struct sockaddr*) &sin_server,
		                 sizeof(sin_server));
		if(ret == -1 && errno == EWOULDBLOCK)
			ret = 0;

		if(ret == -1) {
			printf("Socket error, errno: %d\n", errno);
			break;
		}

		samplesSent += samplesToSend;
		dataBuffer.header.nuFrame++;
	}
}
