#include "ControlInterface.h"
#include "portaudio.h"
#include <string.h>

void onTtyAllocBuffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
	buf->base = new char[suggested_size];
	buf->len = suggested_size;
}
void onTtyRead(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf) {
	ControlInterface* controlInterface = (ControlInterface*) stream->data;
	delete[] static_cast<char*>(buf->base);

	printf("Stopping\n");

	controlInterface->stop();

	uv_read_stop(stream);
	uv_close((uv_handle_t*) stream, nullptr);
}

int main(int argc, char* argv[]) {
	uv_tty_t ttyRead;

	Pa_Initialize();
	ControlInterface controlInterface(argv[0]);

	controlInterface.init("127.0.0.1", 2306);

	controlInterface.loadConfig();

	uv_tty_init(uv_default_loop(), &ttyRead, 0, 1);
	ttyRead.data = &controlInterface;
	uv_read_start((uv_stream_t*) &ttyRead, &onTtyAllocBuffer, &onTtyRead);

	printf("Press enter to stop\n");

	controlInterface.run();

	return 0;
}

void setScheduler() {
#ifndef _WIN32
	struct sched_param sched_param;

	if(sched_getparam(0, &sched_param) < 0) {
		printf("Scheduler getparam failed...\n");
		return;
	}
	sched_param.sched_priority = sched_get_priority_max(SCHED_FIFO);
	if(!sched_setscheduler(0, SCHED_FIFO, &sched_param)) {
		printf("Scheduler set to Round Robin with priority %i...\n", sched_param.sched_priority);
		fflush(stdout);
		return;
	}
	printf("!!!Scheduler set to Round Robin with priority %i FAILED!!!\n", sched_param.sched_priority);
#endif
}
