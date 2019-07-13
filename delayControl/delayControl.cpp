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

	std::string filename = "pulse.wav";

	if(argc > 1) {
		filename = std::string(argv[1]);
	}

	for(int i = 0; i < argc - 2; i++) {
		int instance = atoi(argv[i + 2]);
		outputInstances.push_back(instance);
		printf("Using output instance %d\n", instance);
	}

	if(outputInstances.empty()) {
		outputInstances.push_back(1);
		outputInstances.push_back(0);
		// outputInstances.push_back(5);
	}

	uv_tty_init(uv_default_loop(), &ttyRead, 0, 1);
	ttyRead.data = &delayMeasure;
	uv_read_start((uv_stream_t*) &ttyRead, &onTtyAllocBuffer, &onTtyRead);

	delayMeasure.start(0.20, filename, outputInstances);

	printf("Press enter to stop\n");

	uv_run(uv_default_loop(), UV_RUN_DEFAULT);

	return 0;
}
