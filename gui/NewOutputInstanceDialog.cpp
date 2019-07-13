#include "NewOutputInstanceDialog.h"
#include "ui_NewOutputInstanceDialog.h"
#include <QJsonValue>

NewOutputInstanceDialog::NewOutputInstanceDialog(WavePlayOutputInterface* interface, QWidget* parent)
    : QDialog(parent), ui(new Ui::NewOutputInstanceDialog), interface(interface) {
	ui->setupUi(this);

	connect(ui->buttonBox, SIGNAL(accepted()), this, SLOT(onConfirm()));
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
	ui->deviceDeviceCombo->clear();
	for(QJsonValueRef item : stringArray) {
		ui->deviceDeviceCombo->addItem(item.toString());
	}
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

	interface->sendMessage(json);
	hide();
}
