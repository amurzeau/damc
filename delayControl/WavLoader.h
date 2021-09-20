#pragma once

#include <stdint.h>
#include <string>
#include <vector>

typedef int16_t WAVESAMPLE;

class WavLoader {
public:
	static int load(const std::string& filename, std::vector<WAVESAMPLE>& out, unsigned int& sampleRate);
};
