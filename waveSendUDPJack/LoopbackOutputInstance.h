#ifndef LOOPBACKOUTPUTJACKINSTANCE_H
#define LOOPBACKOUTPUTJACKINSTANCE_H

#include "OutputInstance.h"
#include "json.h"
#include <stdint.h>
// Need to be after else stdint might conflict
#include <jack/jack.h>

class LoopbackOutputInstance : public OutputInstance {
public:
	LoopbackOutputInstance();
	~LoopbackOutputInstance();

protected:
	virtual const char* getName();
	virtual int configureJackPorts(jack_client_t* client, size_t numChannel);
	virtual int processSamples(size_t numChannel, jack_nframes_t nframes);

private:
	std::vector<jack_port_t*> inputPorts;
	std::vector<jack_port_t*> outputPorts;
};

#endif
