#pragma once

#include "Osc/OscEndpoint.h"
#include "OscWidgetMapper.h"
#include "spdlog/sinks/base_sink.h"
#include <QComboBox>
#include <QObject>
#include <QWidget>

template<typename Mutex> class log_sink : public spdlog::sinks::base_sink<Mutex> {
protected:
	void sink_it_(const spdlog::details::log_msg& msg) override {
		spdlog::memory_buf_t formatted;
		spdlog::sinks::base_sink<Mutex>::formatter_->format(msg, formatted);
	}

	void flush_() override {
		// Do nothing because statement executed in sink_it_().
	}
};

namespace Ui {
class LogWindow;
}

class LogWindow : public QWidget, public OscEndpoint {
public:
	explicit LogWindow(QWidget* parent, OscContainer* oscParent);
	~LogWindow();

protected slots:
	void clearLog();

protected:
	void processLog(const std::string& logLine);

private:
	Ui::LogWindow* ui;

	OscWidgetMapper<QComboBox> oscLogLevel;
};
