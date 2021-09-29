#include "RemoteUdpInput.h"
#ifndef _WIN32
#include <netinet/in.h>
#endif
#include "ChannelStrip.h"
#include <algorithm>
#include <limits.h>
#include <string.h>
#include <string>

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

RemoteUdpInput::RemoteUdpInput(OscContainer* oscParent, const char* name)
    : sampleRate(0), started(false), sampleRateMeasure(oscParent, name) {}

RemoteUdpInput::~RemoteUdpInput() {
	stop();
}

static unsigned int highestPowerof2(unsigned int n) {
	unsigned int res = 1;
	while(res < INT_MAX) {
		if(res <= n && res * 2 > n)
			return res;
		res *= 2;
	}
	return res;
}

int RemoteUdpInput::init(int index, const char* ip, int port) {
	std::string streamName = ChannelStrip::JACK_CLIENT_NAME_PREFIX + std::to_string(index);
	uint32_t targetIp = inet_addr(ip);
	struct sockaddr_in sin_server;

	if(started)
		return 1;

	this->sampleRate = 0;
	started = true;

	SPDLOG_INFO("Receiving audio {} on {}:{}", index, ip, port);

	if(!IN_MULTICAST(targetIp))
		sin_server.sin_addr.s_addr = targetIp;
	else
		sin_server.sin_addr.s_addr = INADDR_ANY;

	sin_server.sin_family = AF_INET;
	sin_server.sin_port = htons(port);

	uv_udp_init(uv_default_loop(), &udpSocket);
	udpSocket.data = this;

	sampleRing = jack_ringbuffer_create(highestPowerof2(48000));

	int ret = uv_udp_bind(&udpSocket, (struct sockaddr*) &sin_server, 0);
	if(ret < 0) {
		SPDLOG_ERROR("Bind error on {}:{}: {} ({})", ip, port, uv_strerror(ret), ret);
		return ret;
	}

	if(IN_MULTICAST(targetIp)) {
		ret = uv_udp_set_membership(&udpSocket, ip, nullptr, UV_JOIN_GROUP);
		if(ret < 0) {
			SPDLOG_ERROR("Failed to join multicast group {}: {} ({})", ip, uv_strerror(ret), ret);
			return ret;
		}
	}

	ret = uv_udp_recv_start(&udpSocket, &onAlloc, &onPacketReceived);
	if(ret < 0) {
		SPDLOG_ERROR("Failed to reading UDP: {} ({})", uv_strerror(ret), ret);
		return ret;
	}

	return 0;
}

void RemoteUdpInput::stop() {
	if(started) {
		started = false;
		uv_close((uv_handle_t*) &udpSocket, nullptr);
	}
}

bool RemoteUdpInput::isStarted() {
	return started;
}

void RemoteUdpInput::onSlowTimer() {
	sampleRateMeasure.onTimeoutTimer();
}

void RemoteUdpInput::onAlloc(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
	RemoteUdpInput* thisInstance = (RemoteUdpInput*) handle->data;
	buf->base = (char*) &thisInstance->dataBuffer;
	buf->len = sizeof(thisInstance->dataBuffer);
}

void RemoteUdpInput::onPacketReceived(
    uv_udp_t* handle, ssize_t nread, const uv_buf_t* buf, const sockaddr* addr, unsigned flags) {
	RemoteUdpInput* thisInstance = (RemoteUdpInput*) handle->data;
	float buffer[16384];
	size_t sampleCount;
	size_t channelCount;
	const int16_t* inputSamples;
	const size_t sampleSize = sizeof(float) * 2;

	if(nread <= 0) {
		SPDLOG_WARN("Bad udp read {}", nread);
		return;
	}

	if(memcmp(&thisInstance->dataBuffer.header.vban, "VBAN", 4) == 0) {
		channelCount = thisInstance->dataBuffer.header.format_nbc + 1;
		sampleCount = (int) thisInstance->dataBuffer.header.format_nbs + 1;
		thisInstance->sampleRate = VBAN_SRList[thisInstance->dataBuffer.header.format_SR];
		inputSamples = thisInstance->dataBuffer.data;
	} else {
		channelCount = 2;
		sampleCount = nread / sizeof(int16_t) / channelCount;
		thisInstance->sampleRate = 0;
		inputSamples = (int16_t*) &thisInstance->dataBuffer;
	}

	for(size_t i = 0; i < sampleCount; i++) {
		buffer[i * 2] = inputSamples[i * channelCount] / 32768.0;
		buffer[i * 2 + 1] = inputSamples[i * channelCount + 1] / 32768.0;
	}

	sampleCount = std::min(sampleCount, jack_ringbuffer_write_space(thisInstance->sampleRing) / sampleSize);

	if(sampleCount > 0)
		jack_ringbuffer_write(thisInstance->sampleRing, (const char*) buffer, sampleCount * sampleSize);

	thisInstance->sampleRateMeasure.notifySampleProcessed(sampleCount);
}

size_t RemoteUdpInput::receivePacket(float* samplesLeft, float* samplesRight, size_t maxSamples) {
	float buffer[16384];
	size_t sampleRead;
	size_t sizeToRead;
	const size_t sampleSize = sizeof(samplesLeft[0]) * 2;

	sizeToRead = maxSamples * sampleSize;
	if(sizeToRead > sizeof(buffer))
		sizeToRead = sizeof(buffer);

	// Ensure multiple of sampleSize
	sizeToRead = std::min(sizeToRead, jack_ringbuffer_read_space(sampleRing) / sampleSize * sampleSize);

	sampleRead = jack_ringbuffer_read(sampleRing, (char*) buffer, sizeToRead);
	sampleRead /= sampleSize;

	for(size_t i = 0; i < sampleRead; i++) {
		samplesLeft[i] = buffer[i * 2];
		samplesRight[i] = buffer[i * 2 + 1];
	}

	return sampleRead;
}
