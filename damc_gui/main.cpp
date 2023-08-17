#include "MainWindow.h"
#include "SerialPortInterface.h"
#include "WavePlayInterface.h"
#include <OscRoot.h>
#include <QApplication>
#include <QCommandLineParser>
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
	int exitCode = 1;

	initializeSpdLog();

	OscRoot oscRoot(false);
	bool isMicrocontrollerDamc;

	std::unique_ptr<OscConnector> oscConnector;

	QCommandLineParser parser;

	QCommandLineOption serialOption{"serial", "use serial port instead of TCP/IP socket"};
	QCommandLineOption ipOption{"ip", "IP of the DAMC server", "ip", "127.0.0.1"};
	QCommandLineOption portOption{"port", "Port of the DAMC server", "port", "2408"};

	parser.setApplicationDescription("DAMC");
	parser.addHelpOption();
	parser.addOption(serialOption);
	parser.addOption(ipOption);
	parser.addOption(portOption);

	parser.process(a);

	if(parser.isSet(serialOption)) {
		oscConnector.reset(new SerialPortInterface(&oscRoot));
		isMicrocontrollerDamc = true;
	} else {
		QString ip = parser.value(ipOption);
		QString portStr = parser.value(portOption);
		uint32_t port = portStr.toInt();

		isMicrocontrollerDamc = false;

		if(port == 0) {
			SPDLOG_ERROR("Invalid port {}", portStr.toStdString());
			exitCode = 2;
		} else {
			SPDLOG_INFO("Using DAMC server at {}:{}", ip.toStdString(), port);
			oscConnector.reset(new WavePlayInterface(&oscRoot, ip, port));
		}
	}

	if(oscConnector) {
		MainWindow w(&oscRoot, isMicrocontrollerDamc);
		w.show();

		exitCode = a.exec();
	}

	spdlog::shutdown();

	return exitCode;
}
