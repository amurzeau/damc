#include "SampleRateMeasure.h"
#include <spdlog/spdlog.h>
#include <uv.h>

SampleRateMeasure::SampleRateMeasure(OscContainer* oscParent, const std::string& name)
    : runningData(&data), oscRealSampleRate(oscParent, name) {}

void SampleRateMeasure::notifySampleProcessed(size_t nframes) {
	TimingInfo* data;

	data = runningData.exchange(nullptr);

	if(!data)
		return;

	uint64_t now = uv_hrtime();
	if(data->firstSampleTime == 0)
		data->firstSampleTime = now;
	else
		data->sampleNumber += nframes;

	data->lastSampleTime = now;

	runningData.exchange(data);
}

void SampleRateMeasure::onTimeoutTimer() {
	TimingInfo data;
	TimingInfo* dataPtr;

	dataPtr = runningData.exchange(nullptr);
	if(!dataPtr)
		return;
	data = *dataPtr;
	*dataPtr = TimingInfo{.firstSampleTime = data.lastSampleTime};
	runningData.exchange(dataPtr);

	if(!data.firstSampleTime) {
		SPDLOG_WARN("Jack not running !");
	} else {
		int64_t period = data.lastSampleTime - data.firstSampleTime;

		OscArgument arg = data.sampleNumber / (period / 1000000000.0f);
		oscRealSampleRate.sendMessage(&arg, 1);
	}
}
