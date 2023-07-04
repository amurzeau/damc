#include "MainWindow.h"
#include "SerialPortInterface.h"
#include "WavePlayInterface.h"
#include <OscRoot.h>
#include <QApplication>
#include <spdlog/async.h>
#include <spdlog/cfg/env.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

void initializeSpdLog() {
	spdlog::init_thread_pool(8192, 1);
	std::vector<spdlog::sink_ptr> sinks;

	auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
	console_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%7l%$] %25!!: %v");
	console_sink->set_level(spdlog::level::debug);
	sinks.push_back(console_sink);

	std::shared_ptr<spdlog::sinks::rotating_file_sink_mt> file_sink;
	try {
		file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>("logs/damc_gui.log", 10 * 1024 * 1024, 3);
		SPDLOG_INFO("Logging to logs/damc_gui.log in current directory");
	} catch(...) {
		SPDLOG_INFO("Can't open log file, will log only on console");
	}

	if(file_sink) {
		file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] %7l %t %@: %v");
		file_sink->set_level(spdlog::level::trace);
		sinks.push_back(file_sink);
	}

	/*
	auto logger = std::make_shared<spdlog::async_logger>(
	    "server", sinks.begin(), sinks.end(), spdlog::thread_pool(), spdlog::async_overflow_policy::overrun_oldest);
	//*/
	auto logger = std::make_shared<spdlog::logger>("server", sinks.begin(), sinks.end());
	spdlog::set_default_logger(logger);
	spdlog::set_level(spdlog::level::info);

	spdlog::cfg::load_env_levels();

	spdlog::flush_every(std::chrono::seconds(10));
}

int main(int argc, char* argv[]) {
	QApplication a(argc, argv);

	initializeSpdLog();

	OscRoot oscRoot(false);

	std::unique_ptr<OscConnector> oscConnector;

	qsizetype serialArgIndex = a.arguments().indexOf("--serial");
	if(serialArgIndex >= 0) {
		oscConnector.reset(new SerialPortInterface(&oscRoot));
	} else {
		oscConnector.reset(new WavePlayInterface(&oscRoot));
	}

	MainWindow w(&oscRoot);
	w.show();

	return a.exec();
}
