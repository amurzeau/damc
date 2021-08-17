#include "../filter.h"
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>

#include "../waveSendUDPJack/OutputInstance/ResamplingFilter.h"

#ifdef WIN32
#include <Windows.h>
static LARGE_INTEGER QPCfrequency;
#else
#include <time.h>
#endif

#ifdef HAVE_VALGRIND
#include <valgrind/callgrind.h>
#else
#define CALLGRIND_START_INSTRUMENTATION
#define CALLGRIND_STOP_INSTRUMENTATION
#endif

static void initTimestamp() {
#ifdef WIN32
	QueryPerformanceFrequency(&QPCfrequency);
#endif
}
static uint64_t getCurrentTimestamp() {
#ifdef WIN32
	LARGE_INTEGER t;
	QueryPerformanceCounter(&t);

	return 1000000000.0 * t.QuadPart / QPCfrequency.QuadPart;
#else
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return ts.tv_sec * 1000000000LL + ts.tv_nsec;
#endif
}

#pragma pack(push, 1)
enum WavChunks : uint32_t {
	WC_RiffHeader = 0x46464952,
	WC_WavRiff = 0x45564157,
	WC_Format = 0x020746d66,
	WC_LabeledText = 0x478747C6,
	WC_Instrumentation = 0x478747C6,
	WC_Sample = 0x6C706D73,
	WC_Fact = 0x47361666,
	WC_Data = 0x61746164,
	WC_Junk = 0x4b4e554a,
};

enum WavFormat : uint16_t {
	WF_PulseCodeModulation = 0x01,
	WF_IEEEFloatingPoint = 0x03,
	WF_ALaw = 0x06,
	WF_MuLaw = 0x07,
	WF_IMAADPCM = 0x11,
	WF_YamahaITUG723ADPCM = 0x16,
	WF_GSM610 = 0x31,
	WF_ITUG721ADPCM = 0x40,
	WF_MPEG = 0x50,
	WF_Extensible = 0xFFFE
};

typedef struct {
	uint16_t wFormatTag;
	uint16_t nChannels;
	uint32_t nSamplesPerSec;
	uint32_t nAvgBytesPerSec;
	uint16_t nBlockAlign;
	uint16_t wBitsPerSample;
	uint16_t cbSize;
} WAVEFORMAT2;

typedef struct {
	uint32_t chunkId;
	uint32_t chunkSize;
} WAVECHUNK;

typedef struct {
	float left;
	// float right;
} WAVESAMPLE;

typedef struct {
	WAVECHUNK riffChunk;
	uint32_t riffFormat;
	WAVECHUNK formatChunk;
	WAVEFORMAT2 format;
	WAVECHUNK dataChunk;
} WAVEFILE;
#pragma pack(pop)

static void do_filter_fir(std::vector<WAVESAMPLE>& data);

int main(int argc, char* argv[]) {
	const char* input;
	const char* output;
	FILE* inputFile;
	FILE* outputFile;

	WAVEFORMAT2 format;
	WAVECHUNK chunk;

	initTimestamp();

	uint32_t riffType;
	std::vector<WAVESAMPLE> data;
	int ret = 0;

	if(argc < 3) {
		printf("Usage: %s input output\n", argv[0]);
		return 3;
	}

	input = argv[1];
	output = argv[2];
	inputFile = fopen(input, "rb");
	outputFile = fopen(output, "wb");

	if(!inputFile) {
		printf("Can't open input file %s\n", input);
		return 4;
	}
	if(!outputFile) {
		printf("Can't open output file %s\n", output);
		return 5;
	}

	while(!data.size()) {
		fread(&chunk, sizeof(chunk), 1, inputFile);
		switch((WavChunks) chunk.chunkId) {
			case WC_Format:
				ret =
				    fread(&format, chunk.chunkSize >= sizeof(format) ? sizeof(format) : chunk.chunkSize, 1, inputFile);
				if(ret > 0 && chunk.chunkSize >= sizeof(format)) {
					ret = fseek(inputFile, format.cbSize, SEEK_CUR);
				}
				printf("wFormatTag: %d\n"
				       "nChannels: %d\n"
				       "nSamplesPerSec: %d\n"
				       "nAvgBytesPerSec: %d\n"
				       "nBlockAlign: %d\n"
				       "wBitsPerSample: %d\n",
				       format.wFormatTag,
				       format.nChannels,
				       format.nSamplesPerSec,
				       format.nAvgBytesPerSec,
				       format.nBlockAlign,
				       format.wBitsPerSample);
				if(ret > 0 && (format.wFormatTag != WF_IEEEFloatingPoint || format.nChannels != 1 ||
				               format.wBitsPerSample != 32)) {
					printf("Bad channel number %d or bit per sample %d or format %d\n",
					       format.nChannels,
					       format.wBitsPerSample,
					       format.wFormatTag);
					return 2;
				}
				break;
			case WC_RiffHeader:
				ret = fread(&riffType, sizeof(riffType), 1, inputFile);
				break;
			case WC_Data:
				data.resize(chunk.chunkSize / (format.nChannels * format.wBitsPerSample / 8));
				ret = fread(&data[0], sizeof(data[0]), data.size(), inputFile);
				printf("Read %d samples\n", ret);
				break;
			default:
				ret = fseek(inputFile, chunk.chunkSize, SEEK_CUR);
				break;
		}
		if(ret < 0)
			break;
	}

	fclose(inputFile);
	inputFile = nullptr;
	size_t originalSize = data.size();

	if(!data.size()) {
		printf("Error: %s(%d)\n", strerror(errno), errno);
		return 1;
	}

	/*SampleFilter filter;
	SampleFilter_init(&filter);


	for(size_t i = 0; i < data.size(); i++) {
	    SampleFilter_put(&filter, data[i].left / 32768.0);
	    data[i].right = SampleFilter_get(&filter) * 32768.0;
	}*/

	do_filter_fir(data);
	uint64_t step2 = getCurrentTimestamp();
	// do_filter_iir(data2);
	// uint64_t step3 = getCurrentTimestamp();

	// printf("FIR: %lldus\nIIR: %lldus\n", (step2 - step1)/1000, step3 - step2);

	WAVEFILE file;
	file.riffChunk.chunkId = WC_RiffHeader;
	file.riffChunk.chunkSize = sizeof(file) + data.size() * sizeof(data[0]) - 8;
	file.riffFormat = WC_WavRiff;
	file.formatChunk.chunkId = WC_Format;
	file.formatChunk.chunkSize = sizeof(file.format);
	file.format = format;
	file.format.cbSize = 0;
	file.format.nSamplesPerSec = static_cast<unsigned int>(48000.0 * data.size() / originalSize);
	file.dataChunk.chunkId = WC_Data;
	file.dataChunk.chunkSize = data.size() * sizeof(data[0]);

	fwrite(&file, sizeof(file), 1, outputFile);
	fwrite(&data[0], sizeof(data[0]), data.size(), outputFile);
	fclose(outputFile);

	return 0;
}

static void do_filter_fir(std::vector<WAVESAMPLE>& data) {
	ResamplingFilter filter;
	std::vector<float> outputs;

	filter.reset(48000);

	std::vector<WAVESAMPLE> newData;
	newData.reserve(data.size());
	outputs.reserve(256);

	uint64_t step1 = getCurrentTimestamp();
	CALLGRIND_START_INSTRUMENTATION;
	for(size_t k = 0; k < 1; k++) {
		for(size_t i = 0; i < data.size(); i++) {
			float input = data[i].left;

			filter.put(input);
			outputs.clear();
			filter.get(outputs, 0.5f);

			for(size_t j = 0; j < outputs.size(); j++) {
				WAVESAMPLE s;
				s.left = outputs[j];
				// s.left = input;
				// s.right = outputs[j];
				newData.push_back(s);
			}
		}
	}
	CALLGRIND_STOP_INSTRUMENTATION;
	uint64_t step2 = getCurrentTimestamp();
	printf("FIR: %lldus\n", (step2 - step1) / 1000);
	data.swap(newData);
}
