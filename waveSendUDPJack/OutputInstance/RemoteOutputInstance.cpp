#include "RemoteOutputInstance.h"

void RemoteOutputInstance::stop() {
	remoteUdpOutput.stop();
}

RemoteOutputInstance::RemoteOutputInstance() {
	resamplingFilters.resize(2);
	ip = "127.0.0.1";
	port = 2305;
}

const char* RemoteOutputInstance::getName() {
	return "remote";
}

int RemoteOutputInstance::start(int index, size_t numChannel, int sampleRate, int jackBufferSize) {
	outBuffers[0].resize(jackBufferSize);
	outBuffers[1].resize(jackBufferSize);
	for(ResamplingFilter& resamplingFilter : resamplingFilters) {
		resamplingFilter.reset(sampleRate);
	}
	return remoteUdpOutput.init(index, sampleRate, ip.c_str(), port);
}

nlohmann::json RemoteOutputInstance::getParameters() {
	return nlohmann::json::object(
	    {{"ip", ip}, {"port", port}, {"clockDrift", resamplingFilters[0].getClockDrift()}, {"vban", addVbanHeader}});
}

void RemoteOutputInstance::setParameters(const nlohmann::json& json) {
	ip = json.value("ip", ip);
	port = json.value("port", port);
	addVbanHeader = json.value("vban", addVbanHeader);

	auto clockDrift = json.find("clockDrift");
	if(clockDrift != json.end()) {
		for(auto& resamplingFilter : resamplingFilters)
			resamplingFilter.setClockDrift(clockDrift.value().get<float>());
	}
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

	if(addVbanHeader)
		remoteUdpOutput.sendAudio(resampledBuffer[0].data(), resampledBuffer[1].data(), resampledBuffer[0].size());
	else
		remoteUdpOutput.sendAudioWithoutVBAN(
		    resampledBuffer[0].data(), resampledBuffer[1].data(), resampledBuffer[0].size());

	return 0;
}
