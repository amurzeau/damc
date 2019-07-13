#ifndef BINAURALSPACIALIZER_H

#include <atomic>
#include <memory>
#include <string>
#include <uv.h>
#include <vector>

#include <jack/jack.h>
#include <mysofa.h>

#include "Convolver.h"
#include "DelayFilter.h"

class BinauralSpacializer {
public:
	BinauralSpacializer();
	int start(unsigned int numChannels, const std::string& pulseFileName);
	void stop();

protected:
	static int processSamplesStatic(jack_nframes_t nframes, void* arg);
	int processSamples(jack_nframes_t nframes);

	void fillTransforms(struct MYSOFA_EASY* mysofaHandle, int filterLength, size_t numChannels);

private:
	struct SourceToEarTransformation {
		struct EarTransform {
			Convolver hrtf;
			DelayFilter delay;
		};

		std::array<EarTransform, 2> ears;
	};

	jack_client_t* client;
	std::vector<jack_port_t*> inputPorts;
	std::array<jack_port_t*, 2> outputPorts;

	std::unique_ptr<struct MYSOFA_EASY, void (*)(struct MYSOFA_EASY* easy)> mysofaHandle;
	std::vector<SourceToEarTransformation> transforms;

	float sampleRate;
};

#endif
