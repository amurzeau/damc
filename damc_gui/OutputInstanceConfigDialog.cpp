#include "OutputInstanceConfigDialog.h"
#include "MainWindow.h"
#include "OutputController.h"
#include "QtUtils.h"
#include "ui_OutputInstanceConfigDialog.h"

OutputInstanceConfigDialog::OutputInstanceConfigDialog(MainWindow* mainWindow, OutputController* parent)
    : ManagedVisibilityWidget<QDialog>(parent),
      OscContainer(parent, "device"),
      ui(new Ui::OutputInstanceConfigDialog),
      mainWindow(mainWindow),
      oscType(parent, "_type"),
      oscChannelNumber(parent, "channels"),
      oscReverseAudioSignal(parent->getFilterChain(), "reverseAudioSignal"),
      oscTinyDenoiserFilter(parent->getFilterChain(), "tinyDenoiserFilter"),
      oscTinyDenoiserEnable(&oscTinyDenoiserFilter, "enable"),
      oscTinyDenoiserRatio(&oscTinyDenoiserFilter, "ratio"),
      oscTinyDenoiserUseNpu(&oscTinyDenoiserFilter, "useNpu"),
      oscDeviceName(this, "deviceName"),
      oscBufferSize(this, "bufferSize"),
      oscActualBufferSize(this, "actualBufferSize"),
      oscClockDrift(this, "clockDrift"),
      oscMeasuredClockDrift(this, "measuredClockDrift"),
      oscDeviceSampleRate(this, "deviceSampleRate"),
      oscIp(this, "ip"),
      oscPort(this, "port"),
      oscAddVbanHeader(this, "vbanFormat"),
      oscExclusiveMode(this, "exclusiveMode"),
      oscIsRunning(this, "isRunning"),
      oscUnderflowCount(this, "underflowCount"),
      oscOverflowCount(this, "overflowCount"),
      oscDeviceRealSampleRate(this, "realSampleRate") {
	ui->setupUi(this);

	connect(ui->typeCombo, qOverload<int>(&QComboBox::activated), this, &OutputInstanceConfigDialog::updateDeviceCombo);

	connect(mainWindow, &MainWindow::typeListChanged, this, &OutputInstanceConfigDialog::updateTypeCombo);
	connect(mainWindow, &MainWindow::deviceListChanged, this, &OutputInstanceConfigDialog::updateDeviceCombo);

	updateTypeCombo();
	updateDeviceCombo();

	parent->getOscName()->setWidget(ui->nameEdit);
	parent->getOscDisplayName()->setWidget(ui->displayNameEdit);

	oscClockDrift.setScale(1000000);

	parent->getOscEnable()->setWidget(ui->enableCheckBox);
	oscType.setWidget(ui->typeCombo);
	oscChannelNumber.setWidget(ui->channelsSpin);
	oscReverseAudioSignal.setWidget(ui->reverseAudioSignalCheckBox);
	oscTinyDenoiserEnable.setWidget(ui->tinyDenoiserEnableCheckBox);
	oscTinyDenoiserRatio.setWidget(ui->tinyDenoiserRatioSpinBox);
	oscTinyDenoiserUseNpu.setWidget(ui->tinyDenoiserUseNpuCheckBox);
	oscDeviceName.setWidget(ui->deviceDeviceCombo);
	oscBufferSize.setWidget(ui->bufferSizeSpinBox);
	oscActualBufferSize.setWidget(ui->actualBufferSize);
	oscClockDrift.setWidget(ui->clockDriftSpinBox);
	oscMeasuredClockDrift.setWidget(ui->measuredClockDriftSpinBox);
	oscDeviceSampleRate.setWidget(ui->sampleRateSpinBox);
	oscIp.setWidget(ui->remoteIpEdit);
	oscPort.setWidget(ui->remotePortSpin);
	oscAddVbanHeader.setWidget(ui->vbanCheckBox);
	oscExclusiveMode.setWidget(ui->useExclusiveModeCheckBox);
	oscIsRunning.setWidget(ui->isRunningCheckBox);
	oscOverflowCount.setWidget(ui->bufferOverflowCountSpinBox);
	oscUnderflowCount.setWidget(ui->bufferUnderflowCountSpinBox);
	parent->getOscJackSampleRate()->setWidget(ui->measuredJackSampleRateSpinBox);
	oscDeviceRealSampleRate.setWidget(ui->measuredDeviceSampleRateSpinBox);

	oscType.addChangeCallback([this](int) { updateGroupBoxes(); });

	manageWidgetsVisiblity();
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
	emit showStateChanged(true);
}

void OutputInstanceConfigDialog::hideEvent(QHideEvent*) {
	emit showStateChanged(false);
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

	ui->audioParametersGroupBox->setVisible(clockConfigEnable);
	ui->remoteOutputBox->setVisible(remoteConfigEnable);
	ui->deviceOutputBox->setVisible(deviceConfigEnable);
	ui->useExclusiveModeCheckBox->setVisible(deviceExclusiveModeConfigEnable);
	adjustSize();
}
