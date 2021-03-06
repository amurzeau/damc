#include "RemoteOutputInstance.h"

RemoteOutputInstance::RemoteOutputInstance(OscContainer* parent)
    : OscContainer(parent, "device"),
      remoteUdpOutput(this, "realSampleRate"),
      oscIp(this, "ip", "127.0.0.1"),
      oscPort(this, "port", 2305),
      oscDeviceSampleRate(this, "deviceSampleRate", 48000),
      oscClockDrift(this, "clockDrift", 0.0f),
      oscAddVbanHeader(this, "vbanFormat") {
	resamplingFilters.resize(2);

	oscIp.addCheckCallback([this](auto) { return !remoteUdpOutput.isStarted(); });
	oscPort.addCheckCallback([this](auto) { return !remoteUdpOutput.isStarted(); });
	oscDeviceSampleRate.addCheckCallback([](int32_t newValue) { return newValue > 0; });

	oscClockDrift.addChangeCallback([this](float newValue) {
		for(auto& resamplingFilter : resamplingFilters) {
			resamplingFilter.setClockDrift(newValue);
		}
	});
	oscDeviceSampleRate.addChangeCallback([this](int32_t newValue) {
		for(auto& resamplingFilter : resamplingFilters) {
			resamplingFilter.setTargetSamplingRate(newValue);
		}
		remoteUdpOutput.setSampleRate(newValue);
	});
}

RemoteOutputInstance::~RemoteOutputInstance() {
	RemoteOutputInstance::stop();
}

const char* RemoteOutputInstance::getName() {
	return "remote";
}

int RemoteOutputInstance::start(int index, size_t numChannel, int sampleRate, int jackBufferSize) {
	outBuffers[0].resize(jackBufferSize);
	outBuffers[1].resize(jackBufferSize);
	for(ResamplingFilter& resamplingFilter : resamplingFilters) {
		resamplingFilter.reset(sampleRate);
		resamplingFilter.setTargetSamplingRate(oscDeviceSampleRate);
		resamplingFilter.setClockDrift(oscClockDrift);
	}
	return remoteUdpOutput.init(index, oscDeviceSampleRate, oscIp.c_str(), oscPort);
}

void RemoteOutputInstance::stop() {
	remoteUdpOutput.stop();
}

void RemoteOutputInstance::onSlowTimer() {
	remoteUdpOutput.onSlowTimer();
}

int RemoteOutputInstance::postProcessSamples(float** samples, size_t numChannel, jack_nframes_t nframes) {
	outBuffers[0].resize(nframes);
	outBuffers[1].resize(nframes);

	if(numChannel == 1) {
		for(jack_nframes_t i = 0; i < nframes; i++) {
			outBuffers[0][i] = outBuffers[1][i] = samples[0][i];
		}
	} else if(numChannel == 2) {
		for(jack_nframes_t i = 0; i < nframes; i++) {
			outBuffers[0][i] = samples[0][i];
			outBuffers[1][i] = samples[1][i];
		}
	} else if(numChannel >= 3 && numChannel < 6) {
		for(jack_nframes_t i = 0; i < nframes; i++) {
			outBuffers[0][i] = (samples[0][i] + 0.707 * samples[2][i]);
			outBuffers[1][i] = (samples[1][i] + 0.707 * samples[2][i]);
		}
	} else if(numChannel >= 6 && numChannel < 8) {
		for(jack_nframes_t i = 0; i < nframes; i++) {
			outBuffers[0][i] = (samples[0][i] + 0.707 * samples[2][i] + samples[4][i]);
			outBuffers[1][i] = (samples[1][i] + 0.707 * samples[2][i] + samples[5][i]);
		}
	} else if(numChannel >= 8) {
		for(jack_nframes_t i = 0; i < nframes; i++) {
			outBuffers[0][i] = (samples[0][i] + 0.707 * samples[2][i] + samples[4][i] + samples[6][i]);
			outBuffers[1][i] = (samples[1][i] + 0.707 * samples[2][i] + samples[5][i] + samples[7][i]);
		}
	}

	for(size_t i = 0; i < resamplingFilters.size(); i++) {
		resamplingFilters[i].processSamples(resampledBuffer[i], outBuffers[i].data(), nframes);
	}

	if(oscAddVbanHeader)
		remoteUdpOutput.sendAudio(resampledBuffer[0].data(), resampledBuffer[1].data(), resampledBuffer[0].size());
	else
		remoteUdpOutput.sendAudioWithoutVBAN(
		    resampledBuffer[0].data(), resampledBuffer[1].data(), resampledBuffer[0].size());

	return 0;
}
