#ifndef REMOTEUDPOUTPUT_H
#define REMOTEUDPOUTPUT_H

#include "FilteringChain.h"
#include <uv.h>

class RemoteUdpOutput {
public:
	RemoteUdpOutput();
	~RemoteUdpOutput();

	int init(int index, int samplerate, const char* ip, int port);

	void sendAudio(int16_t* samplesLeft, int16_t* samplesRight, size_t samplesCount);

private:
#pragma pack(push, 1)
	struct VbanHeader {
		uint32_t vban;       /* contains 'V' 'B', 'A', 'N' */
		uint8_t format_SR;   /* SR index (see SRList above) */
		uint8_t format_nbs;  /* nb sample per frame (1 to 256) */
		uint8_t format_nbc;  /* nb channel (1 to 256) */
		uint8_t format_bit;  /* mask = 0x07 (nb Byte integer from 1 to 4) */
		char streamname[16]; /* stream name */
		uint32_t nuFrame;    /* growing frame number. */
	};
	struct VbanBuffer {
		VbanHeader header;
		int16_t data[1024];
	};
#pragma pack(pop)

private:
	struct sockaddr_in sin_server;
	int sock_fd;
	VbanBuffer dataBuffer;
};

#endif
