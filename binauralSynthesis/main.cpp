#include <stdio.h>
#include <string>
#include <vector>

#include <uv.h>

#include <jack/jack.h>
#include <mysofa.h>

#include "BinauralSpacializer.h"

void onTtyAllocBuffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
	buf->base = new char(suggested_size);
	buf->len = suggested_size;
}
void onTtyRead(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf) {
	BinauralSpacializer* delayMeasure = (BinauralSpacializer*) stream->data;
	free(buf->base);

	printf("Stopping\n");

	delayMeasure->stop();

	uv_read_stop(stream);
}

int main(int argc, char* argv[]) {
	BinauralSpacializer binauralSpacializer;
	uv_tty_t ttyRead;
	const char* filename = "RIEC_hrir_subject_001.sofa";
	unsigned int numChannels = 0;

	if(argc >= 2)
		filename = argv[1];

	if(argc >= 3)
		numChannels = atoi(argv[2]);
	if(!numChannels)
		numChannels = 8;

	uv_tty_init(uv_default_loop(), &ttyRead, 0, 1);
	ttyRead.data = &binauralSpacializer;
	uv_read_start((uv_stream_t*) &ttyRead, &onTtyAllocBuffer, &onTtyRead);

	printf("Running binaural spacializer with %d input channels and HRTFs file %s\n", numChannels, filename);

	binauralSpacializer.start(numChannels, filename);

	printf("Press enter to stop\n");

	uv_run(uv_default_loop(), UV_RUN_DEFAULT);

	return 0;
}
