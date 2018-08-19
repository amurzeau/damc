#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <pulse/introspect.h>
#include <pulse/simple.h>
#include <sched.h>
#include <stdlib.h>
#include <string.h>
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <unistd.h>

#include "Logger.h"
#include <stdio.h>

#define PACKETSIZE 16384

#pragma pack(push, 1)
struct vban_data_t {
	uint32_t vban;       /* contains 'V' 'B', 'A', 'N' */
	uint8_t format_SR;   /* SR index (see SRList above) */
	uint8_t format_nbs;  /* nb sample per frame (1 to 256) */
	uint8_t format_nbc;  /* nb channel (1 to 256) */
	uint8_t format_bit;  /* mask = 0x07 (nb Byte integer from 1 to 4) */
	char streamname[16]; /* stream name */
	uint32_t nuFrame;    /* growing frame number. */
	int16_t data[];
};
#pragma pack(pop)

#define VBAN_PROTOCOL_AUDIO 0x00
#define VBAN_PROTOCOL_SERIAL 0x20
#define VBAN_PROTOCOL_TXT 0x40
#define VBAN_PROTOCOL_SERVICE 0x60
#define VBAN_PROTOCOL_UNDEFINED_1 0x80
#define VBAN_PROTOCOL_UNDEFINED_2 0xA0
#define VBAN_PROTOCOL_UNDEFINED_3 0xC0
#define VBAN_PROTOCOL_USER 0xE

void setScheduler();
// bool setHwParams(snd_pcm_t* hWave, int numChannels, int bytesPerSample, int samplePerSec, int period, int numBuffer);
// void dumpParams(snd_pcm_t* hWave);

inline int min(int a, int b) {
	return (a > b) ? b : a;
}

bool inputAvailable() {
	struct timeval tv;
	fd_set fds;
	tv.tv_sec = 0;
	tv.tv_usec = 0;
	FD_ZERO(&fds);
	FD_SET(STDIN_FILENO, &fds);
	select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv);
	return (FD_ISSET(0, &fds));
}

int main(int argc, char* argv[]) {
	pa_simple* hWaveIn;
	pa_sample_spec sampleFormat;
	pa_buffer_attr bufferAttrs;
	int result;

	int datasent;
	struct vban_data_t* data_buffer;
	double volume = 1;

	int connection = -1;
	struct sockaddr_in sin_server = {0};

	const char* ip = NULL;
	const char* audioTarget = NULL;
	int port = 2305;
	int audioSamplePerSec = 48000;
	int audioChannels = 2;
	int audioBytesPerSample = 2;
	int audioNumBuffer = 4;

	int bufferChunk = 128;
	int bufferSize;
	int packetSize;

	for(int i = 1; i < argc; i++) {
		if(!strcmp(argv[i], "--rate") && (i + 1) < argc) {
			audioSamplePerSec = atoi(argv[i + 1]);
			i++;
		} else if(!strcmp(argv[i], "--channel") && (i + 1) < argc) {
			audioChannels = atoi(argv[i + 1]);
			i++;
		} else if(!strcmp(argv[i], "--chunksize") && (i + 1) < argc) {
			bufferChunk = atoi(argv[i + 1]);
			i++;
		} else if(!strcmp(argv[i], "--chunknum") && (i + 1) < argc) {
			audioNumBuffer = atoi(argv[i + 1]);
			i++;
		} else if(!strcmp(argv[i], "--port") && (i + 1) < argc) {
			port = atoi(argv[i + 1]);
			i++;
		} else if(!strcmp(argv[i], "--device") && (i + 1) < argc) {
			audioTarget = argv[i + 1];
			i++;
		} else {
			ip = argv[i];
		}
	}

	if(ip == NULL) {
		printf("Usage example: ./waveSendUDP --chunksize 128 --chunknum 4 --device "
		       "hw:0,0 --rate 48000 --channel 2 --port 2305 192.168.1.10\n");
		return 1;
	}

	bufferSize = bufferChunk * audioChannels * audioBytesPerSample;

	printf("Sending audio to %s:%d\n", ip, port);
	setScheduler();

	sin_server.sin_addr.s_addr = inet_addr(ip);
	sin_server.sin_family = AF_INET;
	sin_server.sin_port = htons(port);

	if((connection = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		logMessage("Erreur Winsock");
		return 2;
	}

	int flags;
	flags = fcntl(connection, F_GETFL, 0);
	fcntl(connection, F_SETFL, flags | O_NONBLOCK);
	flags = 1;
	setsockopt(connection, SOL_SOCKET, SO_BROADCAST, &flags, sizeof(flags));

	// Get default output
	sampleFormat.format = PA_SAMPLE_S16LE;
	sampleFormat.channels = audioChannels;
	sampleFormat.rate = audioSamplePerSec;
	bufferAttrs.fragsize = bufferSize;
	bufferAttrs.maxlength = audioNumBuffer * bufferSize;
	bufferAttrs.minreq = -1;
	bufferAttrs.prebuf = -1;
	bufferAttrs.tlength = -1;

	hWaveIn = pa_simple_new(
	    NULL, "waveSendUDPPulse", PA_STREAM_RECORD, audioTarget, "record", &sampleFormat, NULL, &bufferAttrs, &result);
	if(hWaveIn == NULL) {
		logMessage("Failed to open waveform input device: %d.", result);
		return 3;
	}

	printf("Record latency: %lu\n", pa_simple_get_latency(hWaveIn, NULL));

	packetSize = bufferSize + sizeof(struct vban_data_t);
	data_buffer = (struct vban_data_t*) malloc(packetSize);

	data_buffer->vban = 0x4e414256;  // "VBAN"
	data_buffer->format_SR = 3;      // 48Khz
	data_buffer->format_nbc = 1;     // 2-1 channel
	data_buffer->format_bit = 1;     // INT16 PCM
	strcpy(data_buffer->streamname, "Linux");
	data_buffer->nuFrame = 0;

	logMessage("Recording device %s...\n", audioTarget);
	while(1) {
		if(inputAvailable()) {
			char line[256];
			read(STDIN_FILENO, line, 255);
			line[255] = 0;
			if(isdigit(line[0]))
				volume = atof(line);
			printf("vol = %lf\n", volume);
		}

		if(pa_simple_read(hWaveIn, data_buffer->data, bufferSize, &result) < 0) {
			printf("Error %d\n", result);
		} else {
			result = 0;
		}

		if(result < 0) {
			printf("snd_pcm_... failed\n");
			continue;
		}
		short max = 0;
		if(volume != 1.0f) {
			for(int i = 0; i < bufferSize / 2; i++) {
				data_buffer->data[i] = volume * data_buffer->data[i];
				if(max < data_buffer->data[i])
					max = data_buffer->data[i];
			}
		}
		if(max > 20000)
			printf("max = %d\n", max);
		if(result == 0) {
			int ret;
			for(datasent = 0; datasent < bufferChunk;) {
				if(bufferChunk - datasent <= 256)
					data_buffer->format_nbs = (bufferChunk - datasent) - 1;
				else
					data_buffer->format_nbs = 255;
				ret = sendto(
				    connection,
				    (char*) data_buffer,
				    sizeof(struct vban_data_t) + (data_buffer->format_nbs + 1) * audioChannels * audioBytesPerSample,
				    MSG_NOSIGNAL,
				    (struct sockaddr*) &sin_server,
				    sizeof(sin_server));
				if(ret == -1 && errno == EWOULDBLOCK)
					ret = 0;

				if(ret == -1) {
					printf("Socket error, errno: %d\n", errno);
					break;
				}
				datasent += data_buffer->format_nbs + 1;
				if(bufferChunk > datasent)
					memmove(data_buffer->data,
					        data_buffer->data + (data_buffer->format_nbs + 1) * audioChannels * audioBytesPerSample,
					        bufferChunk - datasent);
				data_buffer->nuFrame++;
			}
		}
	}

	close(connection);
	pa_simple_free(hWaveIn);
	return 0;
}

void setScheduler() {
	struct sched_param sched_param;

	if(sched_getparam(0, &sched_param) < 0) {
		printf("Scheduler getparam failed...\n");
		return;
	}
	sched_param.sched_priority = sched_get_priority_max(SCHED_FIFO);
	if(!sched_setscheduler(0, SCHED_FIFO, &sched_param)) {
		printf("Scheduler set to Round Robin with priority %i...\n", sched_param.sched_priority);
		fflush(stdout);
		return;
	}
	printf("!!!Scheduler set to Round Robin with priority %i FAILED!!!\n", sched_param.sched_priority);
}
