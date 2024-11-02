#pragma once

#include "ManagedVisibilityWidget.h"
#include "OscWidgetMapper.h"
#include "qcheckbox.h"
#include <QDialog>

namespace Ui {
class GlobalConfigDialog;
}

class GlobalConfigDialog : public ManagedVisibilityWidget<QDialog> {
	Q_OBJECT

public:
	explicit GlobalConfigDialog(QWidget* parent, OscContainer* oscParent);
	~GlobalConfigDialog();

	void resetGlitchCounters();

signals:
	void showStateChanged(bool shown);
	void glitchDetectionSupported();
	void glitchOccurred();

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

	OscContainer oscGlitches;
	OscWidgetMapper<QAbstractButton> oscGlitchesResetCounters;
	std::array<OscWidgetMapper<QSpinBox>, 9> glitchesCounters;
	std::array<OscWidgetMapper<QAbstractButton>, 2> feedbackClockSync;
};
