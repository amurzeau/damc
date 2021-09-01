#include "RemoteInputInstance.h"

void RemoteInputInstance::stop() {
	remoteUdpInput.stop();
}

RemoteInputInstance::RemoteInputInstance(OscContainer* parent)
    : OscContainer(parent, "device"),
      oscIp(this, "ip", "127.0.0.1"),
      oscPort(this, "port", 2305),
      oscClockDrift(this, "clockDrift", 0.0f) {
	direction = D_Input;

	oscIp.addCheckCallback([this](auto) { return !remoteUdpInput.isStarted(); });
	oscPort.addCheckCallback([this](auto) { return !remoteUdpInput.isStarted(); });

	oscClockDrift.setChangeCallback([this](float newValue) {
		for(auto& resamplingFilter : resamplingFilters) {
			resamplingFilter.setClockDrift(newValue);
		}
	});
}

RemoteInputInstance::~RemoteInputInstance() {
	RemoteInputInstance::stop();
}

const char* RemoteInputInstance::getName() {
	return "remote-in";
}

int RemoteInputInstance::start(int index, size_t numChannel, int sampleRate, int jackBufferSize) {
	this->sampleRate = sampleRate;
	resamplingFilters.resize(2);
	resampledBuffer[0].resize(sampleRate);
	resampledBuffer[1].resize(sampleRate);
	inBuffers[0].reserve(sampleRate);
	inBuffers[1].reserve(sampleRate);
	for(ResamplingFilter& resamplingFilter : resamplingFilters) {
		resamplingFilter.reset(sampleRate);
	}
	return remoteUdpInput.init(index, sampleRate, oscIp.c_str(), oscPort);
}

int RemoteInputInstance::postProcessSamples(float** samples, size_t numChannel, jack_nframes_t nframes) {
	int inputSampleRate = remoteUdpInput.getSampleRate();

	size_t readSize =
	    remoteUdpInput.receivePacket(resampledBuffer[0].data(), resampledBuffer[1].data(), resampledBuffer[0].size());

	for(size_t i = 0; i < resamplingFilters.size(); i++) {
		for(size_t j = 0; j < readSize; j++) {
			resamplingFilters[i].put(resampledBuffer[i][j]);
			resamplingFilters[i].get(inBuffers[i],
			                         resamplingFilters[i].getOverSamplingRatio() /
			                             ((1.0 + (double) oscClockDrift) * this->sampleRate / inputSampleRate));
		}
	}

	if(numChannel == 1) {
		for(size_t i = 0; i < nframes; i++) {
			float left = i < inBuffers[0].size() ? inBuffers[0][i] : 0;
			float right = i < inBuffers[1].size() ? inBuffers[1][i] : 0;
			samples[0][i] = (left + right) / 2;
		}
	} else {
		for(size_t i = 0; i < nframes; i++) {
			samples[0][i] = i < inBuffers[0].size() ? inBuffers[0][i] : 0;
			samples[1][i] = i < inBuffers[1].size() ? inBuffers[1][i] : 0;
		}
		for(size_t i = 2; i < numChannel; i++) {
			std::fill_n(samples[i], nframes, 0);
		}
	}

	size_t eraseSize = nframes;
	if(eraseSize > inBuffers[0].size())
		eraseSize = inBuffers[0].size();
	inBuffers[0].erase(inBuffers[0].begin(), inBuffers[0].begin() + eraseSize);
	inBuffers[1].erase(inBuffers[1].begin(), inBuffers[1].begin() + eraseSize);

	return 0;
}
