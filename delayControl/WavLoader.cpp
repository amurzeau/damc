#include "WavLoader.h"
#include <errno.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>

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
	WAVECHUNK riffChunk;
	uint32_t riffFormat;
	WAVECHUNK formatChunk;
	WAVEFORMAT2 format;
	WAVECHUNK dataChunk;
} WAVEFILE;

int WavLoader::load(const std::string& filename, std::vector<WAVESAMPLE>& data, unsigned int& sampleRate) {
	FILE* inputFile;

	WAVEFORMAT2 format;
	WAVECHUNK chunk;

	uint32_t riffType;
	int ret = 0;

	inputFile = fopen(filename.c_str(), "rb");

	if(!inputFile) {
		printf("Can't open input file %s\n", filename.c_str());
		return -4;
	}

	data.clear();

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
				if(ret > 0 && (format.nChannels != 1 || format.wBitsPerSample != 16)) {
					printf("Bad channel number %d or bit per sample %d\n", format.nChannels, format.wBitsPerSample);
					return -2;
				}
				sampleRate = format.nSamplesPerSec;
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

	return 0;
}
