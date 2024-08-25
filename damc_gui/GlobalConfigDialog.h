#pragma once

#include "ManagedVisibilityWidget.h"
#include "OscWidgetMapper.h"
#include "WidgetAutoHidden.h"
#include <QDialog>

namespace Ui {
class GlobalConfigDialog;
}

class GlobalConfigDialog : public ManagedVisibilityWidget<QDialog> {
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
	void updateCpuTotalUsagePerLoop();
	void updateMemoryUsagePercent();

private:
	Ui::GlobalConfigDialog* ui;

	OscWidgetMapper<QAbstractButton> oscEnableAutoConnect;
	OscWidgetMapper<QAbstractButton> oscMonitorConnections;

	OscContainer oscConfigContainer;
	OscWidgetMapper<QSpinBox> oscSaveCount;
	OscWidgetMapper<QAbstractButton> oscSaveNow;
	OscWidgetMapper<QAbstractButton> oscEnableMicBias;

	OscContainer oscCpuContainer;
	OscWidgetMapper<QDoubleSpinBox> cpuFrequency;
	OscWidgetMapper<QComboBox> cpuDivider;
	OscWidgetMapper<QAbstractButton> cpuManualControl;

	OscWidgetMapper<QDoubleSpinBox> timeUsbInterrupt;
	OscWidgetMapper<QDoubleSpinBox> timeAudioProcessing;
	OscWidgetMapper<QDoubleSpinBox> timeOtherInterrupts;
	OscWidgetMapper<QDoubleSpinBox> timeMainLoop;
	OscWidgetMapper<QDoubleSpinBox> timePerLoopUsbInterrupt;
	OscWidgetMapper<QDoubleSpinBox> timePerLoopAudioProcessing;
	OscWidgetMapper<QDoubleSpinBox> timePerLoopOtherInterrupts;
	OscWidgetMapper<QDoubleSpinBox> timePerLoopMainLoop;

	OscWidgetMapper<QSpinBox> fastMemoryUsed;
	OscWidgetMapper<QSpinBox> fastMemoryAvailable;
	OscWidgetMapper<QSpinBox> slowMemoryUsed;
	OscWidgetMapper<QSpinBox> slowMemoryAvailable;
};
