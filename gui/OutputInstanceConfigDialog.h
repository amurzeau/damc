#ifndef DIALOG_H
#define DIALOG_H

#include "OscWidgetMapper.h"
#include <Osc/OscContainer.h>
#include <Osc/OscVariable.h>
#include <QDialog>

namespace Ui {
class OutputInstanceConfigDialog;
}

class MainWindow;

class OutputInstanceConfigDialog : public QDialog, public OscContainer {
	Q_OBJECT

public:
	explicit OutputInstanceConfigDialog(MainWindow* mainWindow, OscContainer* oscParent, QWidget* parent = 0);
	~OutputInstanceConfigDialog();

protected slots:
	void onConfirm();
	void updateTypeCombo();
	void updateDeviceCombo();

protected:
	void showEvent(QShowEvent*) override;
	void updateGroupBoxes();

private:
	Ui::OutputInstanceConfigDialog* ui;
	MainWindow* mainWindow;

	OscWidgetMapper<QComboBox> oscType;
	OscWidgetMapper<QSpinBox> oscChannelNumber;
	OscWidgetMapper<QComboBox, std::string> oscDeviceName;
	OscWidgetMapper<QDoubleSpinBox> oscClockDrift;
	OscWidgetMapper<QSpinBox> oscDeviceSampleRate;
	OscWidgetMapper<QLineEdit> oscIp;
	OscWidgetMapper<QSpinBox> oscPort;
	OscWidgetMapper<QAbstractButton> oscAddVbanHeader;
	OscWidgetMapper<QAbstractButton> oscExclusiveMode;
};

#endif  // DIALOG_H
