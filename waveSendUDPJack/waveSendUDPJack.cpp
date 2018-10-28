#include "ControlInterface.h"
#include <string.h>

int main(int argc, char* argv[]) {
	const char* ip = NULL;
	int port = 2305;

	for(int i = 1; i < argc; i++) {
		if(!strcmp(argv[i], "--port") && (i + 1) < argc) {
			port = atoi(argv[i + 1]);
			i++;
		} else {
			ip = argv[i];
		}
	}

	if(ip == NULL) {
		printf("Usage example: %s --port 2305 192.168.1.10\n", argv[0]);
		return 1;
	}

	ControlInterface controlInterface(argv[0]);

	controlInterface.init("127.0.0.1", 2306);
	controlInterface.addRemoteOutput(ip, port);
	controlInterface.addLocalOutput();

	controlInterface.loadConfig();

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
