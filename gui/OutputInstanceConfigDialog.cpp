#include "OutputInstanceConfigDialog.h"
#include "MainWindow.h"
#include "OutputController.h"
#include "ui_OutputInstanceConfigDialog.h"

OutputInstanceConfigDialog::OutputInstanceConfigDialog(MainWindow* mainWindow, OutputController* parent)
    : QDialog(parent),
      OscContainer(parent, "device"),
      ui(new Ui::OutputInstanceConfigDialog),
      mainWindow(mainWindow),
      oscType(parent, "_type"),
      oscChannelNumber(parent, "channels"),
      oscDeviceName(this, "deviceName"),
      oscClockDrift(this, "clockDrift"),
      oscDeviceSampleRate(this, "deviceSampleRate"),
      oscIp(this, "ip"),
      oscPort(this, "port"),
      oscAddVbanHeader(this, "vbanFormat"),
      oscExclusiveMode(this, "exclusiveMode") {
	ui->setupUi(this);

	connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &OutputInstanceConfigDialog::onConfirm);
	connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &OutputInstanceConfigDialog::hide);
	connect(ui->typeCombo, qOverload<int>(&QComboBox::activated), this, &OutputInstanceConfigDialog::updateDeviceCombo);

	connect(mainWindow, &MainWindow::typeListChanged, this, &OutputInstanceConfigDialog::updateTypeCombo);
	connect(mainWindow, &MainWindow::deviceListChanged, this, &OutputInstanceConfigDialog::updateDeviceCombo);

	updateTypeCombo();
	updateDeviceCombo();

	parent->getOscName()->setWidget(ui->nameEdit);
	parent->getOscDisplayName()->setWidget(ui->displayNameEdit);

	oscType.setWidget(ui->typeCombo);
	oscChannelNumber.setWidget(ui->channelsSpin);
	oscDeviceName.setWidget(ui->deviceDeviceCombo);
	oscClockDrift.setWidget(ui->clockDriftSpinBox);
	oscDeviceSampleRate.setWidget(ui->sampleRateSpinBox);
	oscIp.setWidget(ui->remoteIpEdit);
	oscPort.setWidget(ui->remotePortSpin);
	oscAddVbanHeader.setWidget(ui->vbanCheckBox);
	oscExclusiveMode.setWidget(ui->useExclusiveModeCheckBox);

	oscClockDrift.setScale(1000000);

	oscType.setChangeCallback([this](int) { updateGroupBoxes(); });
}

OutputInstanceConfigDialog::~OutputInstanceConfigDialog() {
	delete ui;
}

void OutputInstanceConfigDialog::onConfirm() {
	printf("Not implemented\n");
	hide();
}

void OutputInstanceConfigDialog::updateTypeCombo() {
	ui->typeCombo->blockSignals(true);

	ui->typeCombo->clear();
	for(const std::string& item : mainWindow->getTypeList()) {
		printf("Adding type %s\n", item.c_str());
		ui->typeCombo->addItem(QString::fromStdString(item));
	}
	ui->typeCombo->setCurrentIndex(oscType.get());

	ui->typeCombo->blockSignals(false);
}

void OutputInstanceConfigDialog::updateDeviceCombo() {
	ui->deviceDeviceCombo->blockSignals(true);

	ui->deviceDeviceCombo->clear();
	if(ui->typeCombo->currentText().contains("Wasapi")) {
		for(const std::string& item : mainWindow->getDeviceWasapiList()) {
			ui->deviceDeviceCombo->addItem(QString::fromStdString(item));
		}
	} else {
		for(const std::string& item : mainWindow->getDeviceList()) {
			ui->deviceDeviceCombo->addItem(QString::fromStdString(item));
		}
	}
	ui->deviceDeviceCombo->setCurrentText(QString::fromStdString(oscDeviceName.get()));
	int index = ui->deviceDeviceCombo->findText(QString::fromStdString(oscDeviceName.get()));
	if(index >= 0)
		ui->deviceDeviceCombo->setCurrentIndex(index);

	ui->deviceDeviceCombo->blockSignals(false);
}

void OutputInstanceConfigDialog::showEvent(QShowEvent*) {
	updateTypeCombo();
	updateDeviceCombo();
	mainWindow->updateDeviceList();
}

void OutputInstanceConfigDialog::updateGroupBoxes() {
#ifdef _WIN32
#define WASAPI_TYPES(_) \
	_(WasapiDeviceOutput) \
	_(WasapiDeviceInput)
#else
#define WASAPI_TYPES(_)
#endif

#define OUTPUT_INSTANCE_VALID_TYPES(_) \
	_(Loopback) \
	_(RemoteOutput) \
	_(RemoteInput) \
	_(DeviceOutput) \
	_(DeviceInput) \
	WASAPI_TYPES(_)

#define OUTPUT_INSTANCE_TYPES(_) \
	OUTPUT_INSTANCE_VALID_TYPES(_) \
	_(None)

	enum Type {
#define ENUM_ITEM(item) item,
		OUTPUT_INSTANCE_TYPES(ENUM_ITEM)
#undef ENUM_ITEM
		    MaxType
	};

	bool clockConfigEnable = false;
	bool remoteConfigEnable = false;
	bool deviceConfigEnable = false;
	bool deviceExclusiveModeConfigEnable = false;

	switch(oscType.get()) {
		case Loopback:
			break;
		case RemoteOutput:
		case RemoteInput:
			clockConfigEnable = true;
			remoteConfigEnable = true;
			break;
		case DeviceOutput:
		case DeviceInput:
			clockConfigEnable = true;
			deviceConfigEnable = true;
#ifdef _WIN32
			deviceExclusiveModeConfigEnable = true;
#endif
			break;
#ifdef _WIN32
		case WasapiDeviceOutput:
		case WasapiDeviceInput:
			clockConfigEnable = true;
			deviceConfigEnable = true;
			deviceExclusiveModeConfigEnable = true;
			break;
#endif
		case None:
			break;
	}

	ui->sampleRateBox->setVisible(clockConfigEnable);
	ui->remoteOutputBox->setVisible(remoteConfigEnable);
	ui->deviceOutputBox->setVisible(deviceConfigEnable);
	ui->useExclusiveModeCheckBox->setVisible(deviceExclusiveModeConfigEnable);
	adjustSize();
}
