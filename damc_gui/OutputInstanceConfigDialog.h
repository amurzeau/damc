#pragma once

#include "OscWidgetMapper.h"
#include <Osc/OscContainer.h>
#include <Osc/OscVariable.h>
#include <QDialog>

namespace Ui {
class OutputInstanceConfigDialog;
}

class MainWindow;
class OutputController;

class OutputInstanceConfigDialog : public QDialog, public OscContainer {
	Q_OBJECT

public:
	explicit OutputInstanceConfigDialog(MainWindow* mainWindow, OutputController* parent);
	~OutputInstanceConfigDialog();

protected slots:
	void onConfirm();
	void updateTypeCombo();
	void updateDeviceCombo();

signals:
	void showStateChanged(bool shown);

protected:
	void showEvent(QShowEvent*) override;
	void hideEvent(QHideEvent*) override;
	void updateGroupBoxes();

private:
	Ui::OutputInstanceConfigDialog* ui;
	MainWindow* mainWindow;

	OscWidgetMapper<QComboBox> oscType;
	OscWidgetMapper<QSpinBox> oscChannelNumber;
	OscWidgetMapper<QComboBox, std::string> oscDeviceName;
	OscWidgetMapper<QSpinBox> oscBufferSize;
	OscWidgetMapper<QSpinBox> oscActualBufferSize;
	OscWidgetMapper<QDoubleSpinBox> oscClockDrift;
	OscWidgetMapper<QSpinBox> oscDeviceSampleRate;
	OscWidgetMapper<QLineEdit> oscIp;
	OscWidgetMapper<QSpinBox> oscPort;
	OscWidgetMapper<QAbstractButton> oscAddVbanHeader;
	OscWidgetMapper<QAbstractButton> oscExclusiveMode;

	OscWidgetMapper<QSpinBox> oscDeviceRealSampleRate;
};
