#include "LoopbackOutputInstance.h"

const char* LoopbackOutputInstance::getName() {
	return "loopback";
}

int LoopbackOutputInstance::postProcessSamples(float** samples, size_t numChannel, uint32_t nframes) {
	return 0;
}
