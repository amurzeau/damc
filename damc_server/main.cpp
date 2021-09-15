#include "ControlInterface.h"
#include <portaudio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <xmmintrin.h>

#include <pa_debugprint.h>

#include "spdlog/cfg/env.h"
#include "spdlog/sinks/rotating_file_sink.h"
#include <spdlog/async.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

void onTtyAllocBuffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
	buf->base = new char[suggested_size];
	buf->len = suggested_size;
}

void onTtyRead(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf) {
	ControlInterface* controlInterface = (ControlInterface*) stream->data;
	delete[] static_cast<char*>(buf->base);

	SPDLOG_INFO("Enter pressed, stopping");

	controlInterface->stop();

	uv_read_stop(stream);
	uv_close((uv_handle_t*) stream, nullptr);
}

void initializeSpdLog() {
	spdlog::init_thread_pool(8192, 1);
	auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
	console_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%5l%$] %25!: %v");
	console_sink->set_level(spdlog::level::info);

	auto file_sink =
	    std::make_shared<spdlog::sinks::rotating_file_sink_mt>("logs/damc_server.log", 10 * 1024 * 1024, 3);
	file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] %5l %t %@: %v");
	file_sink->set_level(spdlog::level::trace);

	std::vector<spdlog::sink_ptr> sinks{console_sink, file_sink};
	///*
	auto logger = std::make_shared<spdlog::async_logger>(
	    "server", sinks.begin(), sinks.end(), spdlog::thread_pool(), spdlog::async_overflow_policy::overrun_oldest);
	//*/\
	auto logger = std::make_shared<spdlog::logger>("server", sinks.begin(), sinks.end());
	spdlog::set_default_logger(logger);
	spdlog::set_level(spdlog::level::debug);

	spdlog::cfg::load_env_levels();

	spdlog::flush_every(std::chrono::seconds(10));
}

void portAudioLogger(const char* log) {
	size_t logSize = strlen(log);
	while(logSize > 0 && (log[logSize - 1] == '\r' || log[logSize - 1] == '\n'))
		logSize--;

	SPDLOG_DEBUG("{}", std::string_view(log, logSize));
}

int main() {
	uv_tty_t ttyRead;

	unsigned int oldMXCSR = _mm_getcsr();      /* read the old MXCSR setting */
	unsigned int newMXCSR = oldMXCSR | 0x8040; /* set DAZ and FZ bits */
	_mm_setcsr(newMXCSR);                      /* write the new MXCSR setting to the MXCSR */

	srand(time(nullptr));

	initializeSpdLog();

	SPDLOG_INFO("Initializing portaudio");

	PaUtil_SetDebugPrintFunction(&portAudioLogger);
	Pa_Initialize();

	SPDLOG_INFO("Initializing DAMC server");

	ControlInterface controlInterface;
	if(controlInterface.init("127.0.0.1", 2406) != 0) {
		SPDLOG_ERROR("Initialization failed, can't start");
		return 1;
	}

	if(uv_tty_init(uv_default_loop(), &ttyRead, 0, 1) == 0) {
		ttyRead.data = &controlInterface;
		uv_read_start((uv_stream_t*) &ttyRead, &onTtyAllocBuffer, &onTtyRead);
		SPDLOG_INFO("Press enter to stop");
	}

	controlInterface.run();

	spdlog::shutdown();

	return 0;
}

void setScheduler() {
#ifndef _WIN32
	struct sched_param sched_param;

	if(sched_getparam(0, &sched_param) < 0) {
		SPDLOG_WARN("Scheduler getparam failed");
		return;
	}
	sched_param.sched_priority = sched_get_priority_max(SCHED_FIFO);
	if(!sched_setscheduler(0, SCHED_FIFO, &sched_param)) {
		SPDLOG_INFO("Scheduler set to Round Robin with priority {}", sched_param.sched_priority);
		return;
	}
	SPDLOG_ERROR("Scheduler set to Round Robin with priority {} FAILED", sched_param.sched_priority);
#endif
}
