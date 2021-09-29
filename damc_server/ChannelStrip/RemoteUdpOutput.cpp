#include "RemoteUdpOutput.h"
#ifdef _WIN32
#include <WinSock2.h>
#include <Windows.h>
#else
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#endif
#include "ChannelStrip.h"
#include <string.h>
#include <string>

static uint32_t VBAN_SRList[] = {6000,   12000,  24000,  48000, 96000, 192000, 384000, 8000,   16000,  32000, 64000,
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

RemoteUdpOutput::RemoteUdpOutput(OscContainer* oscParent, const char* name)
    : started(false), sampleRateMeasure(oscParent, name) {}

RemoteUdpOutput::~RemoteUdpOutput() {
	stop();
}

int RemoteUdpOutput::init(int index, int samplerate, const char* ip, int port) {
	std::string streamName = ChannelStrip::JACK_CLIENT_NAME_PREFIX + std::to_string(index);
	uint32_t targetIp = inet_addr(ip);

	if(started)
		return 1;

	started = true;
	SPDLOG_INFO("Sending audio {} to {}:{}", index, ip, port);

	sin_server.sin_addr.s_addr = targetIp;

	sin_server.sin_family = AF_INET;
	sin_server.sin_port = htons(port);

	if((sock_fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		SPDLOG_ERROR("Cannot create UDP socket: {} ({})", strerror(errno), errno);
		return 2;
	}

#ifndef _WIN32
	int flags;
	flags = fcntl(sock_fd, F_GETFL, 0);
	fcntl(sock_fd, F_SETFL, flags | O_NONBLOCK);
	flags = 1;
	setsockopt(sock_fd, SOL_SOCKET, SO_BROADCAST, &flags, sizeof(flags));
#else
	unsigned long flags = 1;
	ioctlsocket(sock_fd, FIONBIO, &flags);
#endif

	dataBuffer.header.vban = 0x4e414256;  // "VBAN"
	setSampleRate(samplerate);
	dataBuffer.header.format_nbc = 2 - 1;                // numChannels - 1 channel
	dataBuffer.header.format_bit = VBAN_DATATYPE_INT16;  // INT16 PCM
	strncpy(dataBuffer.header.streamname, streamName.c_str(), sizeof(dataBuffer.header.streamname) - 1);
	dataBuffer.header.nuFrame = 0;

	return 0;
}

void RemoteUdpOutput::stop() {
	if(started) {
		started = false;
		int sock_fd = this->sock_fd;
		this->sock_fd = -1;
#ifdef _WIN32
		closesocket(sock_fd);
#else
		close(sock_fd);
#endif
	}
}

bool RemoteUdpOutput::isStarted() {
	return started;
}

void RemoteUdpOutput::onSlowTimer() {
	sampleRateMeasure.onTimeoutTimer();
}

void RemoteUdpOutput::setSampleRate(uint32_t sampleRate) {
	for(size_t i = 0; i < sizeof(VBAN_SRList) / sizeof(VBAN_SRList[0]); i++) {
		if(VBAN_SRList[i] == sampleRate) {
			dataBuffer.header.format_SR = i;
			break;
		}
	}
}

void RemoteUdpOutput::sendAudio(float* samplesLeft, float* samplesRight, size_t samplesCount) {
	if(sock_fd <= 0)
		return;

	for(size_t samplesSent = 0; samplesSent < samplesCount;) {
		size_t samplesToSend = samplesCount - samplesSent;
		if(samplesToSend > 256)
			samplesToSend = 256;

		dataBuffer.header.format_nbs = samplesToSend - 1;

		for(size_t i = 0; i < samplesToSend; i++) {
			dataBuffer.data[i * 2] = samplesLeft[i + samplesSent] * 32768;
			dataBuffer.data[i * 2 + 1] = samplesRight[i + samplesSent] * 32768;
		}

		int ret = sendto(sock_fd,
		                 (char*) &dataBuffer,
		                 sizeof(dataBuffer.header) + samplesToSend * 2 * sizeof(dataBuffer.data[0]),
		                 0,
		                 (const struct sockaddr*) &sin_server,
		                 sizeof(sin_server));
		if(ret == -1 && errno == EWOULDBLOCK)
			ret = 0;

		if(ret == -1 && sock_fd > 0) {
			SPDLOG_ERROR("Socket error, errno: {}", errno);
			break;
		}

		samplesSent += samplesToSend;
		dataBuffer.header.nuFrame++;
	}

	sampleRateMeasure.notifySampleProcessed(samplesCount);
}

void RemoteUdpOutput::sendAudioWithoutVBAN(float* samplesLeft, float* samplesRight, size_t samplesCount) {
	if(sock_fd <= 0)
		return;

	for(size_t samplesSent = 0; samplesSent < samplesCount;) {
		size_t samplesToSend = samplesCount - samplesSent;
		if(samplesToSend > sizeof(dataBuffer.data))
			samplesToSend = sizeof(dataBuffer.data);

		for(size_t i = 0; i < samplesToSend; i++) {
			dataBuffer.data[i * 2] = samplesLeft[i + samplesSent] * 32768;
			dataBuffer.data[i * 2 + 1] = samplesRight[i + samplesSent] * 32768;
		}

		int ret = sendto(sock_fd,
		                 (char*) dataBuffer.data,
		                 samplesToSend * 2 * sizeof(dataBuffer.data[0]),
		                 0,
		                 (const struct sockaddr*) &sin_server,
		                 sizeof(sin_server));
		if(ret == -1 && errno == EWOULDBLOCK)
			ret = 0;

		if(ret == -1 && sock_fd > 0) {
			// SPDLOG_INFO("Socket error, errno: {}", errno);
		}

		samplesSent += samplesToSend;
	}

	sampleRateMeasure.notifySampleProcessed(samplesCount);
}
