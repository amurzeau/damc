#include "GlobalConfigDialog.h"
#include "ui_GlobalConfigDialog.h"
#include <Osc/OscContainer.h>

GlobalConfigDialog::GlobalConfigDialog(QWidget* parent, OscContainer* oscParent)
    : QDialog(parent),
      ui(new Ui::GlobalConfigDialog),
      oscEnableAutoConnect(oscParent, "enableAutoConnect"),
      oscMonitorConnections(oscParent, "enableConnectionMonitoring"),
      oscSaveCount(oscParent, "saveCount"),
      oscSaveNow(oscParent, "saveNow"),
      timeUsbInterrupt(oscParent, "timeUsbInterrupt"),
      timeAudioProcessing(oscParent, "timeAudioProc"),
      timeFastTimer(oscParent, "timeFastTimer"),
      timeOscInput(oscParent, "timeOscInput"),
      timePerLoopUsbInterrupt(oscParent, "timePerLoopUsbInterrupt"),
      timePerLoopAudioProcessing(oscParent, "timePerLoopAudioProc"),
      timePerLoopFastTimer(oscParent, "timePerLoopFastTimer"),
      timePerLoopOscInput(oscParent, "timePerLoopOscInput"),
      memoryUsed(oscParent, "memoryUsed"),
      memoryAvailable(oscParent, "memoryAvailable") {
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

	memoryUsed.setWidget(ui->allocatedMemorySpinBox, false);
	memoryUsed.addChangeCallback([this](int32_t value) { updateMemoryUsagePercent(); });

	memoryAvailable.setWidget(ui->availableMemorySpinBox, false);
	memoryAvailable.addChangeCallback([this](int32_t value) { updateMemoryUsagePercent(); });
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
	int32_t totalMemory = memoryUsed.get() + memoryAvailable.get();

	if(totalMemory > 0) {
		ui->allocatedMemoryPercentSpinBox->setValue(100.f * memoryUsed.get() / totalMemory);
		ui->availableMemoryPercentSpinBox->setValue(100.f * memoryAvailable.get() / totalMemory);
	} else {
		ui->allocatedMemoryPercentSpinBox->setValue(0);
		ui->availableMemoryPercentSpinBox->setValue(0);
	}
}