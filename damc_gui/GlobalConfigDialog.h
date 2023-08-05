#pragma once

#include "OscWidgetMapper.h"
#include <QDialog>

namespace Ui {
class GlobalConfigDialog;
}

class GlobalConfigDialog : public QDialog {
	Q_OBJECT

public:
	explicit GlobalConfigDialog(QWidget* parent, OscContainer* oscParent);
	~GlobalConfigDialog();

signals:
	void showStateChanged(bool shown);

protected:
	void showEvent(QShowEvent*) override;
	void hideEvent(QHideEvent*) override;
	void updateCpuTotalUsage();
	void updateMemoryUsagePercent();

private:
	Ui::GlobalConfigDialog* ui;

	OscWidgetMapper<QAbstractButton> oscEnableAutoConnect;
	OscWidgetMapper<QAbstractButton> oscMonitorConnections;

	OscWidgetMapper<QSpinBox> oscSaveCount;
	OscWidgetMapper<QAbstractButton> oscSaveNow;

	OscWidgetMapper<QDoubleSpinBox> timeUsbInterrupt;
	OscWidgetMapper<QDoubleSpinBox> timeAudioProcessing;
	OscWidgetMapper<QDoubleSpinBox> timeFastTimer;
	OscWidgetMapper<QDoubleSpinBox> timeOscInput;
	OscWidgetMapper<QDoubleSpinBox> timePerLoopUsbInterrupt;
	OscWidgetMapper<QDoubleSpinBox> timePerLoopAudioProcessing;
	OscWidgetMapper<QDoubleSpinBox> timePerLoopFastTimer;
	OscWidgetMapper<QDoubleSpinBox> timePerLoopOscInput;
	OscWidgetMapper<QSpinBox> memoryUsed;
	OscWidgetMapper<QSpinBox> memoryAvailable;
};
