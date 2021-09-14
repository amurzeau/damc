#include "DelayMeasure.h"
#include <stdio.h>
#include <string>
#include <vector>

#include <uv.h>

#include <jack/jack.h>

void onTtyAllocBuffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
	buf->base = new char(suggested_size);
	buf->len = suggested_size;
}
void onTtyRead(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf) {
	DelayMeasure* delayMeasure = (DelayMeasure*) stream->data;
	free(buf->base);

	printf("Stopping\n");

	delayMeasure->stop();

	uv_read_stop(stream);
}

int main(int argc, char* argv[]) {
	DelayMeasure delayMeasure;
	uv_tty_t ttyRead;
	std::vector<int> outputInstances;

	std::string filename;
	int thresholdRatio;

	printf("Usage: delayControl pulse.wav threshold_ratio <speaker2 index name> ... <speakerN index name>\n");
	printf("Example for x8 threshold ratio and controlling devices 19 and 20 (name, not display name):\n"
	       "  delayControl pulse.wav 8 19 20\n");

	if(argc < 3)
		return 1;

	filename = std::string(argv[1]);
	thresholdRatio = atoi(argv[2]);

	if(thresholdRatio <= 0) {
		printf("Threshold ratio can't be <= 0\n");
		return 2;
	}

	for(int i = 0; i < argc - 3; i++) {
		int instance = atoi(argv[i + 3]);
		outputInstances.push_back(instance);
		printf("Using output instance %d\n", instance);
	}

	if(outputInstances.empty()) {
		printf("Indicate at least one speaker index to control\n");
		return 3;
	}

	uv_tty_init(uv_default_loop(), &ttyRead, 0, 1);
	ttyRead.data = &delayMeasure;
	uv_read_start((uv_stream_t*) &ttyRead, &onTtyAllocBuffer, &onTtyRead);

	delayMeasure.start(0.20, filename, thresholdRatio, outputInstances);

	printf("Press enter to stop\n");

	uv_run(uv_default_loop(), UV_RUN_DEFAULT);

	return 0;
}
