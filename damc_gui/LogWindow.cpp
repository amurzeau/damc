#include "LogWindow.h"
#include "qpushbutton.h"
#include "spdlog/common.h"
#include "ui_LogWindow.h"
#include <QDateTime>
#include <spdlog/spdlog.h>

LogWindow::LogWindow(QWidget* parent, OscContainer* oscParent)
    : QWidget(parent), OscEndpoint(oscParent, "log"), ui(new Ui::LogWindow), oscLogLevel(oscParent, "log_level") {
	ui->setupUi(this);

	connect(ui->clearButton, &QPushButton::click, this, &LogWindow::clearLog);

	this->setCallback([this](const std::vector<OscArgument>& arguments) {
		if(arguments.size() != 1) {
			SPDLOG_WARN("Invalid number of argument {} for {}, expected only one argument with log string",
			            arguments.size(),
			            this->getFullAddress());
		}
		if(arguments.size() < 1) {
			return;
		}
		std::string logLine;
		if(!OscNode::getArgumentAs<std::string>(arguments[0], logLine)) {
			SPDLOG_ERROR("Failed to get argument of {} as a string", this->getFullAddress());
			return;
		}

		processLog(logLine);
	});
}

LogWindow::~LogWindow() {
	delete ui;
}

void LogWindow::clearLog() {
	ui->logTextEdit->clear();
}

void LogWindow::processLog(const std::string& logLine) {
	QString formatedLogLine =
	    QString("%1: %2\n")
	        .arg(QDateTime::currentDateTime().toString("yyyy/MM/dd hh:mm:ss.zzz"), QString::fromStdString(logLine));
	ui->logTextEdit->appendPlainText(formatedLogLine);
}
