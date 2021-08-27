#ifndef LOOPBACKOUTPUTJACKINSTANCE_H
#define LOOPBACKOUTPUTJACKINSTANCE_H

#include "IAudioEndpoint.h"
#include <stdint.h>
// Need to be after else stdint might conflict
#include <jack/jack.h>

class LoopbackOutputInstance : public IAudioEndpoint {
public:
	virtual const char* getName() override;
	virtual int postProcessSamples(float** samples, size_t numChannel, uint32_t nframes) override;
};

#endif
