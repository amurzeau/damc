#ifndef AUDIOENDPOINT_H
#define AUDIOENDPOINT_H

#include <stdint.h>
#include <uv.h>

class IAudioEndpoint {
public:
	enum Direction { D_Output, D_Input };

	virtual ~IAudioEndpoint() {}
	virtual const char* getName() = 0;
	virtual int start(int index, size_t numChannel, int sampleRate, int jackBufferSize) { return 0; }
	virtual void stop() {}
	virtual void onFastTimer() {}
	virtual void onSlowTimer() {}

	virtual int postProcessSamples(float** samples, size_t numChannel, uint32_t nframes) = 0;

	Direction direction = D_Output;
};

#endif
