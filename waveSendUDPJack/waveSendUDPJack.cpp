#include "ControlInterface.h"
#include "portaudio.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <xmmintrin.h>

void onTtyAllocBuffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
	buf->base = new char[suggested_size];
	buf->len = suggested_size;
}
void onTtyRead(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf) {
	ControlInterface* controlInterface = (ControlInterface*) stream->data;
	delete[] static_cast<char*>(buf->base);

	printf("Stopping, nread: %d\n", (int) nread);

	controlInterface->stop();

	uv_read_stop(stream);
	uv_close((uv_handle_t*) stream, nullptr);
}

int main(int argc, char* argv[]) {
	uv_tty_t ttyRead;

	unsigned int oldMXCSR = _mm_getcsr();      /* read the old MXCSR setting */
	unsigned int newMXCSR = oldMXCSR | 0x8040; /* set DAZ and FZ bits */
	_mm_setcsr(newMXCSR);                      /* write the new MXCSR setting to the MXCSR */

	srand(time(nullptr));
	Pa_Initialize();
	ControlInterface controlInterface;

	controlInterface.init("127.0.0.1", 2406);

	controlInterface.loadConfig();

	if(uv_tty_init(uv_default_loop(), &ttyRead, 0, 1) == 0) {
		ttyRead.data = &controlInterface;
		uv_read_start((uv_stream_t*) &ttyRead, &onTtyAllocBuffer, &onTtyRead);
		printf("Press enter to stop\n");
	}

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
