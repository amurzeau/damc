#ifndef REMOTEUDPINPUT_H
#define REMOTEUDPINPUT_H

#include <atomic>
#include <jack/ringbuffer.h>
#include <uv.h>

class RemoteUdpInput {
public:
	RemoteUdpInput();
	~RemoteUdpInput();

	int init(int index, int samplerate, const char* ip, int port);
	void stop();

	size_t receivePacket(float* samplesLeft, float* samplesRight, size_t maxSamples);
	int getSampleRate() { return sampleRate; }

protected:
	static void onAlloc(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf);
	static void onPacketReceived(
	    uv_udp_t* handle, ssize_t nread, const uv_buf_t* buf, const struct sockaddr* addr, unsigned flags);

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
		int16_t data[65536];
	};
#pragma pack(pop)

private:
	VbanBuffer dataBuffer;
	uv_udp_t udpSocket;
	jack_ringbuffer_t* sampleRing;
	std::atomic<int> sampleRate;
};

#endif
