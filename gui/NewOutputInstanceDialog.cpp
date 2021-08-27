#include "NewOutputInstanceDialog.h"
#include "ui_NewOutputInstanceDialog.h"

NewOutputInstanceDialog::NewOutputInstanceDialog(OscContainer* oscParent, QWidget* parent)
    : QDialog(parent),
      ui(new Ui::NewOutputInstanceDialog),
      oscTypeArray(oscParent, "type_list"),
      oscPortaudioDeviceArray(oscParent, "device_list"),
      oscWasapiDeviceArray(oscParent, "device_list_wasapi") {
	ui->setupUi(this);

	connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &NewOutputInstanceDialog::onConfirm);
	connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &NewOutputInstanceDialog::hide);
	connect(ui->typeCombo, qOverload<int>(&QComboBox::activated), this, &NewOutputInstanceDialog::onDeviceTypeChanged);

	oscTypeArray.setChangeCallback([this](const std::vector<std::string>& newValue) {
		ui->typeCombo->clear();
		for(const std::string& item : newValue) {
			ui->typeCombo->addItem(QString::fromStdString(item));
		}
	});
	oscPortaudioDeviceArray.setChangeCallback([this](const std::vector<std::string>& newValue) {
		if(!ui->typeCombo->currentText().contains("Wasapi")) {
			ui->deviceDeviceCombo->clear();
			for(const std::string& item : newValue) {
				ui->deviceDeviceCombo->addItem(QString::fromStdString(item));
			}
		}
	});
	oscWasapiDeviceArray.setChangeCallback([this](const std::vector<std::string>& newValue) {
		if(ui->typeCombo->currentText().contains("Wasapi")) {
			ui->deviceDeviceCombo->clear();
			for(const std::string& item : newValue) {
				ui->deviceDeviceCombo->addItem(QString::fromStdString(item));
			}
		}
	});
}

NewOutputInstanceDialog::~NewOutputInstanceDialog() {
	delete ui;
}

void NewOutputInstanceDialog::onConfirm() {
	printf("Not implemented\n");
	hide();
}

void NewOutputInstanceDialog::onDeviceTypeChanged() {
	ui->deviceDeviceCombo->clear();

	if(ui->typeCombo->currentText().contains("Wasapi")) {
		for(const std::string& item : oscWasapiDeviceArray.getData()) {
			ui->deviceDeviceCombo->addItem(QString::fromStdString(item));
		}
	} else {
		for(const std::string& item : oscPortaudioDeviceArray.getData()) {
			ui->deviceDeviceCombo->addItem(QString::fromStdString(item));
		}
	}
}
