#include <stdio.h>

#include "Logger.h"
#include <arpa/inet.h>
#include <jack/jack.h>
#include <jack/ringbuffer.h>
#include <sched.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#include <vector>

static void setScheduler();
static int configureServer(int port);
static int writeAudio(jack_client_t* hWaveOut, const char* data, int size, int chunkSize);
static jack_client_t* setupAudio(int numChannels);
static void stopAudio(jack_client_t* hWaveOut);

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

int main(int argc, char* argv[]) {
	int port = 2305;
	int audioChannels = 2;
	int driftAdjust = 0;

	for(int i = 1; (i + 1) < argc; i++) {
		if(!strcmp(argv[i], "--channel")) {
			audioChannels = atoi(argv[i + 1]);
			i++;
		} else if(!strcmp(argv[i], "--port")) {
			port = atoi(argv[i + 1]);
			i++;
		} else if(!strcmp(argv[i], "--drift")) {
			driftAdjust = atoi(argv[i + 1]);
			i++;
		}
	}

	int server = configureServer(port);
	if(server == -1)
		return 0;

	setScheduler();

	logMessage("Ready\n");

	struct timeval packetRecvTimeout;

	while(1) {
		fd_set rfds;
		struct timeval timeout;
		// struct timeval tv_tod, tv_ioctl;

		FD_ZERO(&rfds);
		FD_SET(server, &rfds);
		select((int) server + 1, &rfds, 0, 0, NULL);
		logMessage("Incoming data received, starting player\n");

		jack_client_t* hWaveOut = setupAudio(audioChannels);
		if(hWaveOut == NULL)
			continue;

		int dataRecv;
		int dataRecvBufferSize = 32768;
		char* dataRecvBuffer = new char[dataRecvBufferSize];

		logMessage("Playing...\n");

		int chunkSize = audioChannels * 2;
		int64_t sampleCounter = 0;
		int64_t drift = driftAdjust;

		FD_ZERO(&rfds);
		FD_SET(server, &rfds);

		packetRecvTimeout.tv_usec = 0;
		packetRecvTimeout.tv_sec = 1;  // 1sec de timeout avant de stopper le system de son (quand on recoit plus rien)
		timeout = packetRecvTimeout;
		while(select((int) server + 1,
		             &rfds,
		             0,
		             0,
		             &timeout)) {  // recupere les données de l'autre programme (waveSend) par le reseau
			if((dataRecv = recvfrom(server, dataRecvBuffer, dataRecvBufferSize, 0, NULL, NULL)) <= 0)
				break;

			struct vban_data_t* vban_header = (struct vban_data_t*) dataRecvBuffer;

			int sampleCount;
			short* samples;
			if(vban_header->vban == 0x4e414256) {
				sampleCount = (dataRecv - 28) / chunkSize;
				samples = (short*) (dataRecvBuffer + 28);
			} else {
				sampleCount = dataRecv / chunkSize;
				samples = (short*) dataRecvBuffer;
			}

			sampleCounter += sampleCount;
			int sampleToInsert = sampleCounter * drift / 1000000000ll;

			if((sampleToInsert > 0 && sampleCount >= 2) || (sampleToInsert < 0 && sampleCount >= -sampleToInsert)) {
				if(sampleToInsert > 0) {
					int sourceSample[audioChannels];
					int targetSample[audioChannels];
					for(int i = 0; i < audioChannels; i++) {
						sourceSample[i] = samples[(sampleCount - 2) * audioChannels + i];
						targetSample[i] = samples[(sampleCount - 1) * audioChannels + i];
					}
					for(int i = 0; i < (sampleToInsert + 1); i++) {
						for(int j = 0; j < audioChannels; j++) {
							samples[(sampleCount - 1 + i) * audioChannels + j] =
							    sourceSample[j] + (targetSample[j] - sourceSample[j]) * (i + 1) / (sampleToInsert + 1);
						}
					}
				}
				sampleCount += sampleToInsert;
				sampleCounter -= sampleToInsert * 1000000000 / drift;
			}

			writeAudio(hWaveOut, (const char*) samples, sampleCount, chunkSize);

			if(dataRecv % chunkSize != 0)
				logMessage("Warning: received datagram of size %d but chunk size is %d\n", dataRecv, chunkSize);

			FD_ZERO(&rfds);
			FD_SET(server, &rfds);
			timeout = packetRecvTimeout;
		}

		delete[] dataRecvBuffer;

		logMessage("End of connection\n");

		stopAudio(hWaveOut);
	}

	// ferme la connection
	close(server);

	return 0;
}

static void setScheduler() {
	struct sched_param sched_param;

	if(sched_getparam(0, &sched_param) < 0) {
		logMessage("Scheduler getparam failed...\n");
		return;
	}
	sched_param.sched_priority = sched_get_priority_max(SCHED_FIFO);
	if(!sched_setscheduler(0, SCHED_FIFO, &sched_param)) {
		logMessage("Scheduler set to Round Robin with priority %i...\n", sched_param.sched_priority);
		fflush(stdout);
		return;
	}
	logMessage("!!!Scheduler set to Round Robin with priority %i FAILED!!!\n", sched_param.sched_priority);
}

static int configureServer(int port) {
	struct sockaddr_in sin;
	int server = -1;

	sin.sin_addr.s_addr = htonl(INADDR_ANY);
	sin.sin_family = AF_INET;
	sin.sin_port = htons(port);

	if((server = socket(PF_INET, SOCK_DGRAM, 0)) == -1) {
		logMessage("Erreur Winsock\n");
		return -1;
	}

	if(bind(server, (struct sockaddr*) &sin, sizeof(sin)) == -1) {
		logMessage("Erreur: port deja utilisé, choisissez un autre\n");

		return -1;
	}

	return server;
}

static struct jack_thread_info_t {
	std::vector<jack_port_t*> outputPorts;
	jack_ringbuffer_t* ringBuffer;
} thread_info;

int jack_process(jack_nframes_t nframes, void* arg) {
	jack_thread_info_t* info = (jack_thread_info_t*) arg;
	std::vector<jack_default_audio_sample_t*> out;
	static std::vector<int16_t> data;
	size_t numChannel = info->outputPorts.size();

	out.resize(numChannel);
	for(size_t chn = 0; chn < numChannel; chn++) {
		out[chn] = (jack_default_audio_sample_t*) jack_port_get_buffer(info->outputPorts[chn], nframes);
		memset(out[chn], 0, nframes * sizeof(jack_default_audio_sample_t));
	}

	if(jack_ringbuffer_read_space(info->ringBuffer) >= sizeof(int16_t) * nframes * numChannel) {
		data.resize(nframes * numChannel);
		size_t read =
		    jack_ringbuffer_read(info->ringBuffer, (char*) data.data(), sizeof(int16_t) * nframes * numChannel);

		size_t readFrames = read / sizeof(int16_t) / numChannel;

		for(size_t i = 0; i < readFrames; i++) {
			for(size_t chn = 0; chn < numChannel; chn++) {
				out[chn][i] = data[i * numChannel + chn] / 32768.0;
			}
		}

		if(readFrames < nframes)
			printf("bad frames: %lu\n", readFrames);
	} else {
		for(size_t i = 0; i < nframes; i++) {
			for(size_t chn = 0; chn < numChannel; chn++) {
				out[chn][i] = 0;
			}
		}

		printf("bad frames: %lu\n", jack_ringbuffer_read_space(info->ringBuffer));
	}
	return 0;
}

static jack_client_t* setupAudio(int numChannels) {
	jack_client_t* client;
	jack_status_t status;

	client = jack_client_open("wavePlayUDP", JackNullOption, &status);
	if(client == NULL) {
		logMessage("Failed to open jack: %d.", status);
		return NULL;
	}

	thread_info.outputPorts.resize(numChannels);

	for(int i = 0; i < numChannels; i++) {
		char name[64];

		sprintf(name, "output%d", i + 1);
		thread_info.outputPorts[i] = jack_port_register(client, name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
		if(thread_info.outputPorts[i] == 0) {
			logMessage("cannot register output port \"%s\"!", name);
			jack_client_close(client);
			return NULL;
		}
	}

	int bufferSize = jack_get_buffer_size(client) * 10 * 2 * numChannels;

	thread_info.ringBuffer = jack_ringbuffer_create(bufferSize);

	jack_set_process_callback(client, jack_process, &thread_info);

	if(jack_activate(client)) {
		logMessage("cannot activate client");
	}

	return client;
}

static void stopAudio(jack_client_t* hWaveOut) {
	jack_deactivate(hWaveOut);
	jack_client_close(hWaveOut);
	jack_ringbuffer_free(thread_info.ringBuffer);
}

static int writeAudio(jack_client_t* hWaveOut, const char* data, int size, int chunkSize) {
	jack_ringbuffer_write(thread_info.ringBuffer, data, size * chunkSize);

	return 0;
}
