#include <stdio.h>

#include "Logger.h"
#include "filter.h"
#include "filter_fir2.h"
#include <alsa/asoundlib.h>
#include <arpa/inet.h>
#include <sched.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>

static void setScheduler();
static int configureServer(int port);
static void dumpParams(snd_pcm_t* hWave);
static int writeAudio(snd_pcm_t* hWaveOut, const char* data, int size, int chunkSize);
static int audioRecovery(snd_pcm_t* hWaveOut, int err);
static snd_pcm_t* setupAudio(
    const char* audioTarget, int samplePerSec, int numChannels, snd_pcm_uframes_t* bufferChunk, int numBuffers);
static void stopAudio(snd_pcm_t* hWaveOut);
static void setHwParams(
    snd_pcm_t* hWaveOut, int samplePerSec, int numChannels, snd_pcm_uframes_t* bufferChunk, int numBuffers);
static void setSwParams(snd_pcm_t* hWaveOut, snd_pcm_uframes_t bufferChunk, int numBuffers);
/*

FIR filter designed with
http://t-filter.appspot.com

sampling frequency: 96000 Hz

* 0 Hz - 18000 Hz
  gain = 1
  desired ripple = 0.001 dB
  actual ripple = 0.0017438144643438644 dB

* 24000 Hz - 48000 Hz
  gain = 0
  desired attenuation = -60 dB
  actual attenuation = -52.50488873143957 dB

*/

#define FILTER_TAP_NUM 60

static float filter_taps[FILTER_TAP_NUM] = {
    -0.00016468251099241267, -0.0003398014928446548,  0.001242949328700672,    -0.00034437180863040797,
    -0.0014057200978669325,  -0.00011179680674933007, 0.0022350700215351352,   0.000907703852043078,
    -0.003095528045303807,   -0.0024248293594127104,  0.0036910939294729416,   0.004816495267842575,
    -0.003525081363226198,   -0.008059619554657605,   0.0019580866215319313,   0.011857990516750783,
    0.001716540635242086,    -0.015562407085166754,   -0.008195912065959748,   0.018122961314950004,
    0.01814059155469694,     -0.0180055364140552,     -0.032358848210805216,   0.012842181234232013,
    0.05269393953259328,     0.002211913983485687,    -0.08613462098570118,    -0.04490834927479029,
    0.18148229822409895,     0.4106670980442739,      0.4106670980442739,      0.18148229822409895,
    -0.04490834927479029,    -0.08613462098570118,    0.002211913983485687,    0.05269393953259328,
    0.012842181234232013,    -0.032358848210805216,   -0.0180055364140552,     0.01814059155469694,
    0.018122961314950004,    -0.008195912065959748,   -0.015562407085166754,   0.001716540635242086,
    0.011857990516750783,    0.0019580866215319313,   -0.008059619554657605,   -0.003525081363226198,
    0.004816495267842575,    0.0036910939294729416,   -0.0024248293594127104,  -0.003095528045303807,
    0.000907703852043078,    0.0022350700215351352,   -0.00011179680674933007, -0.0014057200978669325,
    -0.00034437180863040797, 0.001242949328700672,    -0.0003398014928446548,  -0.00016468251099241267};

int main(int argc, char* argv[]) {
	const char* audioTarget = "default";
	int port = 2305;
	int audioSamplePerSec = 48000;
	int audioChannels = 2;
	int audioBytesPerSample = 2;
	snd_pcm_uframes_t audioBufferChunk = 128;
	int audioBufferNum = 10;
	int driftAdjust = 31800;
	int RANGE = 125;
	int PWM_FREQUENCY = 200000;

	for(int i = 1; (i + 1) < argc; i++) {
		if(!strcmp(argv[i], "--rate")) {
			audioSamplePerSec = atoi(argv[i + 1]);
			i++;
		} else if(!strcmp(argv[i], "--channel")) {
			audioChannels = atoi(argv[i + 1]);
			i++;
		} else if(!strcmp(argv[i], "--chunksize")) {
			audioBufferChunk = atoi(argv[i + 1]);
			i++;
		} else if(!strcmp(argv[i], "--chunknum")) {
			audioBufferNum = atoi(argv[i + 1]);
			i++;
		} else if(!strcmp(argv[i], "--port")) {
			port = atoi(argv[i + 1]);
			i++;
		} else if(!strcmp(argv[i], "--device")) {
			audioTarget = argv[i + 1];
			i++;
		} else if(!strcmp(argv[i], "--drift")) {
			driftAdjust = atoi(argv[i + 1]);
			i++;
		} else if(!strcmp(argv[i], "--range")) {
			RANGE = atoi(argv[i + 1]);
			i++;
		} else if(!strcmp(argv[i], "--pwm")) {
			PWM_FREQUENCY = atoi(argv[i + 1]);
			i++;
		}
	}

	int server = configureServer(port);
	if(server == -1)
		return 0;
	int chunkSize = audioChannels * audioBytesPerSample;

	setScheduler();

	logMessage("Ready\n");

	struct timeval packetRecvTimeout;

	while(1) {
		fd_set rfds;
		struct timeval timeout;
		struct filter_fir2_t resamplingFilter[2];
		struct filter_farrow_t farrowFilter[2];
		float drift = (1.0 + driftAdjust / 1000000000.0) * PWM_FREQUENCY / audioSamplePerSec / 2;
		// struct timeval tv_tod, tv_ioctl;

		filter_fir2_init(&resamplingFilter[0], filter_taps, FILTER_TAP_NUM, 2, 1);
		filter_fir2_init(&resamplingFilter[1], filter_taps, FILTER_TAP_NUM, 2, 1);
		filter_farrow_init(&farrowFilter[0]);
		filter_farrow_init(&farrowFilter[1]);

		FD_ZERO(&rfds);
		FD_SET(server, &rfds);
		select((int) server + 1, &rfds, 0, 0, NULL);
		logMessage("Incoming data received, starting player\n");

		snd_pcm_t* hWaveOut = setupAudio(audioTarget, PWM_FREQUENCY, audioChannels, &audioBufferChunk, audioBufferNum);
		if(hWaveOut == NULL)
			break;

		int dataRecv;
		int dataRecvBufferSize = 32768;
		char* dataRecvBuffer = new char[dataRecvBufferSize];
		unsigned int* dataRecvBufferResampled =
		    new unsigned int[filter_farrow_get_out_size(dataRecvBufferSize, drift, 0)];
		float samplesIntermediate1_1[2];
		float samplesIntermediate1_2[2];
		float samplesIntermediate2[filter_farrow_get_out_size(2, drift, 0) + 1];
		float integrator[2][2] = {{0}, {0}};
		static const float feedback_coefs[sizeof(integrator) / sizeof(integrator[0])] = {0.493, 1.250};

		// FILE* fileOut = fopen("debug.raw", "wb");
		logMessage("Playing...\n");

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

			int sampleCount = (dataRecv - 28) / chunkSize;
			short* samples = (short*) (dataRecvBuffer + 28);
			unsigned int* samplesOut = dataRecvBufferResampled;

			int i, j;
			for(i = 0, j = 0; i < sampleCount; i++) {
				unsigned int out_size;
				unsigned int k, m;

				filter_fir2_put(&resamplingFilter[0], samples[i * 2] / (32768.0 / 2));
				out_size = filter_fir2_get(&resamplingFilter[0], samplesIntermediate1_1);

				filter_fir2_put(&resamplingFilter[1], samples[i * 2 + 1] / (32768.0 / 2));
				out_size = filter_fir2_get(&resamplingFilter[1], samplesIntermediate1_2);

				for(k = 0, m = 0; k < out_size; k++) {
					unsigned int out_size_2 = filter_farrow_resample_putfloat(
					    &farrowFilter[0], samplesIntermediate1_1[k], drift, 0, samplesIntermediate2, 1);
					for(size_t l = 0; l < out_size_2; l++) {
						integrator[0][0] += samplesIntermediate2[l];

						float roundedOutput = round((integrator[0][1] / 2 + 0.5) * RANGE);

						integrator[0][1] += integrator[0][0] - ((roundedOutput / RANGE - 0.5) * 2) * feedback_coefs[1];
						integrator[0][0] -= ((roundedOutput / RANGE - 0.5) * 2) * feedback_coefs[0];

						samplesOut[(j + m) * 2] = (unsigned int) roundedOutput;
						m++;
					}
				}

				for(k = 0, m = 0; k < out_size; k++) {
					unsigned int out_size_2 = filter_farrow_resample_putfloat(
					    &farrowFilter[1], samplesIntermediate1_2[k], drift, 0, samplesIntermediate2, 1);
					for(size_t l = 0; l < out_size_2; l++) {
						integrator[1][0] += samplesIntermediate2[l];

						float roundedOutput = round((integrator[1][1] / 2 + 0.5) * RANGE);

						integrator[1][1] += integrator[1][0] - ((roundedOutput / RANGE - 0.5) * 2) * feedback_coefs[1];
						integrator[1][0] -= ((roundedOutput / RANGE - 0.5) * 2) * feedback_coefs[0];

						samplesOut[(j + m) * 2 + 1] = (unsigned int) roundedOutput;
						m++;
					}
				}

				j += m;
			}

			writeAudio(hWaveOut, (const char*) samplesOut, j, audioChannels * sizeof(unsigned int));
			/*if(fileOut)
			    fwrite(samplesOut, j, audioChannels * sizeof(unsigned int), fileOut);
*/
			if(dataRecv % chunkSize != 0)
				logMessage("Warning: received datagram of size %d but chunk size is %d\n", dataRecv, chunkSize);

			FD_ZERO(&rfds);
			FD_SET(server, &rfds);
			timeout = packetRecvTimeout;
		}
		/*
		        if(fileOut)
		fclose(fileOut);*/

		delete[] dataRecvBuffer;
		delete[] dataRecvBufferResampled;

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

static snd_pcm_t* setupAudio(
    const char* audioTarget, int samplePerSec, int numChannels, snd_pcm_uframes_t* bufferChunk, int numBuffers) {
	int result;
	snd_pcm_t* hWaveOut;

	result = snd_pcm_open(&hWaveOut, audioTarget, SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK);
	if(result < 0) {
		logMessage("Error no waveout: %s\n", snd_strerror(result));
		return NULL;
	}

	setHwParams(hWaveOut, samplePerSec, numChannels, bufferChunk, numBuffers);
	setSwParams(hWaveOut, *bufferChunk, numBuffers);
	dumpParams(hWaveOut);

	result = snd_pcm_prepare(hWaveOut);
	if(result < 0) {
		logMessage("Error preparing: %s\n", snd_strerror(result));
		return NULL;
	}

	return hWaveOut;
}

static void stopAudio(snd_pcm_t* hWaveOut) {
	snd_pcm_drop(hWaveOut);
	snd_pcm_close(hWaveOut);
}

static void setHwParams(
    snd_pcm_t* hWaveOut, int samplePerSec, int numChannels, snd_pcm_uframes_t* bufferChunk, int numBuffers) {
	snd_pcm_hw_params_t* hwparams;
	int result;

	snd_pcm_hw_params_alloca(&hwparams);

	result = snd_pcm_hw_params_any(hWaveOut, hwparams);
	if(result < 0) {
		logMessage("snd_pcm_hw_params_any failed: %s\n", snd_strerror(result));
		return;
	}

	if(samplePerSec > 48000)
		result = snd_pcm_hw_params_set_format(hWaveOut, hwparams, SND_PCM_FORMAT_U32_LE);
	else
		result = snd_pcm_hw_params_set_format(hWaveOut, hwparams, SND_PCM_FORMAT_S16_LE);
	if(result < 0) {
		logMessage("snd_pcm_hw_params_set_format failed: %s\n", snd_strerror(result));
		return;
	}

	result = snd_pcm_hw_params_set_rate(hWaveOut, hwparams, samplePerSec, 0);
	if(result < 0) {
		logMessage("snd_pcm_hw_params_set_rate failed: %s, %d\n", snd_strerror(result), samplePerSec);
		return;
	}

	result = snd_pcm_hw_params_set_channels(hWaveOut, hwparams, numChannels);
	if(result < 0) {
		logMessage("snd_pcm_hw_params_set_channels failed: %s\n", snd_strerror(result));
		return;
	}

	result = snd_pcm_hw_params_set_access(hWaveOut, hwparams, SND_PCM_ACCESS_RW_INTERLEAVED);
	if(result < 0) {
		logMessage("snd_pcm_hw_params_set_access failed: %s\n", snd_strerror(result));
		return;
	}

	int dir = 0;
	result = snd_pcm_hw_params_set_period_size_near(hWaveOut, hwparams, bufferChunk, &dir);
	if(result < 0) {
		logMessage("snd_pcm_hw_params_set_period_size_near failed: %s\n", snd_strerror(result));
		return;
	}

	snd_pcm_uframes_t totalBufferSize = (*bufferChunk) * numBuffers;
	result = snd_pcm_hw_params_set_buffer_size_near(hWaveOut, hwparams, &totalBufferSize);
	if(result < 0) {
		logMessage("snd_pcm_hw_params_set_buffer_size_near failed: %s\n", snd_strerror(result));
		return;
	}

	result = snd_pcm_hw_params(hWaveOut, hwparams);
	if(result < 0) {
		logMessage("snd_pcm_hw_params failed: %s\n", snd_strerror(result));
		return;
	}
}

static void setSwParams(snd_pcm_t* hWaveOut, snd_pcm_uframes_t bufferChunk, int numBuffers) {
	snd_pcm_sw_params_t* swparams;
	int result;

	snd_pcm_sw_params_alloca(&swparams);

	result = snd_pcm_sw_params_current(hWaveOut, swparams);
	if(result < 0) {
		logMessage("snd_pcm_sw_params_current failed: %s\n", snd_strerror(result));
		return;
	}

	result = snd_pcm_sw_params_set_start_threshold(
	    hWaveOut, swparams, bufferChunk);  // bug in bcm driver ? sometimes, silence samples get inserted
	if(result < 0) {
		logMessage("snd_pcm_sw_params_set_start_threshold failed: %s\n", snd_strerror(result));
		return;
	}

	result = snd_pcm_sw_params(hWaveOut, swparams);
	if(result < 0) {
		logMessage("snd_pcm_sw_params failed: %s\n", snd_strerror(result));
		return;
	}
}

static int writeAudio(snd_pcm_t* hWaveOut, const char* data, int size, int chunkSize) {
	while(size > 0) {
		int err = snd_pcm_writei(hWaveOut, data, size);
		if(err == -EAGAIN) {
			puts("O");
			return 1;
		}

		if(err < 0) {
			if(audioRecovery(hWaveOut, err) < 0) {
				logMessage("Write error: %s\n", snd_strerror(err));
				return 3;
			}
			return 2; /* skip one period */
		}
		data += err * chunkSize;
		size -= err;
	}

	return 0;
}
static int audioRecovery(snd_pcm_t* hWaveOut, int err) {
	// Under-run?
	if(err == -EPIPE) {
		// NOTE: If you see these during playback, you'll have to increase
		// BUFFERSIZE/PERIODSIZE
		// logMessage("underrun\n");
		if((err = snd_pcm_prepare(hWaveOut)) >= 0)
			return 0;
		logMessage("Can't recovery from underrun, prepare failed: %s\n", snd_strerror(err));
	}

	return (err);
}

static void dumpParams(snd_pcm_t* hWave) {
	snd_output_t* out;

	snd_output_stdio_attach(&out, stderr, 0);
	snd_output_printf(out, "dump :\n");
	snd_pcm_dump_setup(hWave, out);
	snd_output_close(out);
}
