#include "NewOutputInstanceDialog.h"
#include "ui_NewOutputInstanceDialog.h"
#include <QJsonValue>

NewOutputInstanceDialog::NewOutputInstanceDialog(QWidget* parent)
    : QDialog(parent), ui(new Ui::NewOutputInstanceDialog) {
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
	printf("Not implemented\n");
	hide();
}

void NewOutputInstanceDialog::onDeviceTypeChanged() {
	ui->deviceDeviceCombo->clear();

	if(ui->typeCombo->currentText().contains("Wasapi")) {
		for(const QJsonValue& item : qAsConst(wasapiDeviceArray)) {
			ui->deviceDeviceCombo->addItem(item.toString());
		}
	} else {
		for(const QJsonValue& item : qAsConst(portaudioDeviceArray)) {
			ui->deviceDeviceCombo->addItem(item.toString());
		}
	}
}
