#include "GlobalConfigDialog.h"
#include "ui_GlobalConfigDialog.h"
#include <Osc/OscContainer.h>

GlobalConfigDialog::GlobalConfigDialog(QWidget* parent, OscContainer* oscParent)
    : ManagedVisibilityWidget<QDialog>(parent),
      ui(new Ui::GlobalConfigDialog),
      oscEnableAutoConnect(oscParent, "enableAutoConnect"),
      oscMonitorConnections(oscParent, "enableConnectionMonitoring"),
      oscConfigContainer(oscParent, "config"),
      oscSaveCount(&oscConfigContainer, "saveCount"),
      oscSaveNow(&oscConfigContainer, "saveNow"),
      timeUsbInterrupt(oscParent, "timeUsbInterrupt"),
      timeAudioProcessing(oscParent, "timeAudioProc"),
      timeFastTimer(oscParent, "timeFastTimer"),
      timeOscInput(oscParent, "timeOscInput"),
      timePerLoopUsbInterrupt(oscParent, "timePerLoopUsbInterrupt"),
      timePerLoopAudioProcessing(oscParent, "timePerLoopAudioProc"),
      timePerLoopFastTimer(oscParent, "timePerLoopFastTimer"),
      timePerLoopOscInput(oscParent, "timePerLoopOscInput"),
      fastMemoryUsed(oscParent, "fastMemoryUsed"),
      fastMemoryAvailable(oscParent, "fastMemoryAvailable"),
      slowMemoryUsed(oscParent, "memoryUsed"),
      slowMemoryAvailable(oscParent, "memoryAvailable") {
	ui->setupUi(this);

	oscEnableAutoConnect.setWidget(ui->autoConnectJackClients);
	oscMonitorConnections.setWidget(ui->monitorConnections);

	oscSaveCount.setWidget(ui->saveCountSpinBox, false);
	oscSaveNow.setWidget(ui->saveNowPushButton, true);

	// Convert 1/1000000 values to percents (1/100)
	timeUsbInterrupt.setScale(1.f / 10000.f);
	timeAudioProcessing.setScale(1.f / 10000.f);
	timeFastTimer.setScale(1.f / 10000.f);
	timeOscInput.setScale(1.f / 10000.f);

	timeUsbInterrupt.setWidget(ui->usbInterruptsSpinBox, false);
	timeAudioProcessing.setWidget(ui->audioProcessingSpinBox, false);
	timeFastTimer.setWidget(ui->timersProcessingSpinBox, false);
	timeOscInput.setWidget(ui->oscProcessingSpinBox, false);

	timeUsbInterrupt.addChangeCallback([this](float) { updateCpuTotalUsage(); });
	timeAudioProcessing.addChangeCallback([this](float) { updateCpuTotalUsage(); });
	timeFastTimer.addChangeCallback([this](float) { updateCpuTotalUsage(); });
	timeOscInput.addChangeCallback([this](float) { updateCpuTotalUsage(); });

	timePerLoopUsbInterrupt.setScale(1.f / 10.f);
	timePerLoopAudioProcessing.setScale(1.f / 10.f);
	timePerLoopFastTimer.setScale(1.f / 10.f);
	timePerLoopOscInput.setScale(1.f / 10.f);

	timePerLoopUsbInterrupt.setWidget(ui->usbInterruptsPerLoopSpinBox, false);
	timePerLoopAudioProcessing.setWidget(ui->audioProcessingPerLoopSpinBox, false);
	timePerLoopFastTimer.setWidget(ui->timersProcessingPerLoopSpinBox, false);
	timePerLoopOscInput.setWidget(ui->oscProcessingPerLoopSpinBox, false);

	fastMemoryUsed.setWidget(ui->usedFastMemorySpinBox, false);
	fastMemoryUsed.addChangeCallback([this](int32_t value) { updateMemoryUsagePercent(); });

	fastMemoryAvailable.setWidget(ui->availableFastMemorySpinBox, false);
	fastMemoryAvailable.addChangeCallback([this](int32_t value) { updateMemoryUsagePercent(); });

	slowMemoryUsed.setWidget(ui->allocatedSlowMemorySpinBox, false);
	slowMemoryUsed.addChangeCallback([this](int32_t value) { updateMemoryUsagePercent(); });

	slowMemoryAvailable.setWidget(ui->availableSlowMemorySpinBox, false);
	slowMemoryAvailable.addChangeCallback([this](int32_t value) { updateMemoryUsagePercent(); });

	manageWidgetsVisiblity();
}

GlobalConfigDialog::~GlobalConfigDialog() {
	delete ui;
}

void GlobalConfigDialog::showEvent(QShowEvent*) {
	emit showStateChanged(true);
}

void GlobalConfigDialog::hideEvent(QHideEvent*) {
	emit showStateChanged(false);
}

void GlobalConfigDialog::updateCpuTotalUsage() {
	float totalCpuUsage = timeUsbInterrupt.get() + timeAudioProcessing.get() + timeFastTimer.get() + timeOscInput.get();
	ui->totalCpuUsageSpinBox->setValue(totalCpuUsage / 10000.f);
}

void GlobalConfigDialog::updateMemoryUsagePercent() {
	int32_t totalMemory = fastMemoryUsed.get() + fastMemoryAvailable.get();

	if(totalMemory > 0) {
		ui->usedFastMemoryPercentSpinBox->setValue(100.f * fastMemoryUsed.get() / totalMemory);
		ui->availableFastMemoryPercentSpinBox->setValue(100.f * fastMemoryAvailable.get() / totalMemory);
	} else {
		ui->usedFastMemoryPercentSpinBox->setValue(0);
		ui->availableFastMemoryPercentSpinBox->setValue(0);
	}

	totalMemory = slowMemoryUsed.get() + slowMemoryAvailable.get();

	if(totalMemory > 0) {
		ui->allocatedSlowMemoryPercentSpinBox->setValue(100.f * slowMemoryUsed.get() / totalMemory);
		ui->availableSlowMemoryPercentSpinBox->setValue(100.f * slowMemoryAvailable.get() / totalMemory);
	} else {
		ui->allocatedSlowMemoryPercentSpinBox->setValue(0);
		ui->availableSlowMemoryPercentSpinBox->setValue(0);
	}
}