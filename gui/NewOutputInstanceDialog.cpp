#include "NewOutputInstanceDialog.h"
#include "ui_NewOutputInstanceDialog.h"
#include <QJsonValue>

NewOutputInstanceDialog::NewOutputInstanceDialog(WavePlayOutputInterface* interface, QWidget* parent)
    : QDialog(parent), ui(new Ui::NewOutputInstanceDialog), interface(interface) {
	ui->setupUi(this);

	connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &NewOutputInstanceDialog::onConfirm);
	connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &NewOutputInstanceDialog::hide);
	connect(ui->typeCombo, qOverload<int>(&QComboBox::activated), this, &NewOutputInstanceDialog::onDeviceTypeChanged);
}

NewOutputInstanceDialog::~NewOutputInstanceDialog() {
	delete ui;
}

void NewOutputInstanceDialog::setTypeList(QJsonArray stringArray) {
	ui->typeCombo->clear();
	for(QJsonValueRef item : stringArray) {
		ui->typeCombo->addItem(item.toString());
	}
}

void NewOutputInstanceDialog::setDeviceList(QJsonArray stringArray) {
	portaudioDeviceArray = stringArray;
}

void NewOutputInstanceDialog::setWasapiDeviceList(QJsonArray stringArray) {
	wasapiDeviceArray = stringArray;
}

void NewOutputInstanceDialog::onConfirm() {
	QJsonObject json;
	json["operation"] = "add";
	json["type"] = ui->typeCombo->currentIndex();
	json["numChannels"] = ui->channelsSpin->value();
	json["ip"] = ui->remoteIpEdit->text();
	json["port"] = ui->remotePortSpin->value();
	json["device"] = ui->deviceDeviceCombo->currentText();
	json["vban"] = ui->vbanCheckBox->isChecked();
	json["sampleRate"] = ui->sampleRateSpinBox->value();
	json["bitPerSample"] = ui->bitPerSampleSpinBox->value();
	json["useExclusiveMode"] = ui->useExclusiveModeCheckBox->isChecked();

	interface->sendMessage(json);
	hide();
}

void NewOutputInstanceDialog::onDeviceTypeChanged() {
	ui->deviceDeviceCombo->clear();

	if(ui->typeCombo->currentText().contains("Wasapi")) {
		for(QJsonValue item : qAsConst(wasapiDeviceArray)) {
			ui->deviceDeviceCombo->addItem(item.toString());
		}
	} else {
		for(QJsonValue item : qAsConst(portaudioDeviceArray)) {
			ui->deviceDeviceCombo->addItem(item.toString());
		}
	}
}
