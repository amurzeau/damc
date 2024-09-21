#include "GlobalConfigDialog.h"
#include "ui_GlobalConfigDialog.h"
#include <Osc/OscContainer.h>
#include <math.h>

GlobalConfigDialog::GlobalConfigDialog(QWidget* parent, OscContainer* oscParent)
    : ManagedVisibilityWidget<QDialog>(parent),
      ui(new Ui::GlobalConfigDialog),
      oscEnableAutoConnect(oscParent, "enableAutoConnect"),
      oscMonitorConnections(oscParent, "enableConnectionMonitoring"),
      oscConfigContainer(oscParent, "config"),
      oscSaveCount(&oscConfigContainer, "saveCount"),
      oscSaveNow(&oscConfigContainer, "saveNow"),
      oscEnableMicBias(oscParent, "enableMicBias", true),
      oscCpuContainer(oscParent, "cpu"),
      cpuFrequency(&oscCpuContainer, "freq"),
      cpuDivider(&oscCpuContainer, "divider"),
      cpuManualControl(&oscCpuContainer, "manual"),
      timeUsbInterrupt(oscParent, "timeUsbInterrupt"),
      timeAudioProcessing(oscParent, "timeAudioProc"),
      timeOtherInterrupts(oscParent, "timeOtherInterrupts"),
      timeMainLoop(oscParent, "timeMainLoop"),
      timePerLoopUsbInterrupt(oscParent, "timePerLoopUsbInterrupt"),
      timePerLoopAudioProcessing(oscParent, "timePerLoopAudioProc"),
      timePerLoopOtherInterrupts(oscParent, "timePerLoopOtherInterrupts"),
      timePerLoopMainLoop(oscParent, "timePerLoopMainLoop"),
      fastMemoryUsed(oscParent, "fastMemoryUsed"),
      fastMemoryAvailable(oscParent, "fastMemoryAvailable"),
      slowMemoryUsed(oscParent, "memoryUsed"),
      slowMemoryAvailable(oscParent, "memoryAvailable"),
      oscGlitches(oscParent, "glitches"),
      oscGlitchesResetCounters(&oscGlitches, "reset"),
      glitchesCounters{
          OscWidgetMapper<QSpinBox>{&oscGlitches, "usbIsochronousTransferLost"},
          OscWidgetMapper<QSpinBox>{&oscGlitches, "usbOutOverrun"},
          OscWidgetMapper<QSpinBox>{&oscGlitches, "usbOutUnderrun"},
          OscWidgetMapper<QSpinBox>{&oscGlitches, "usbInOverrun"},
          OscWidgetMapper<QSpinBox>{&oscGlitches, "audioProcessInterruptLost"},
          OscWidgetMapper<QSpinBox>{&oscGlitches, "codecOutXRun"},
          OscWidgetMapper<QSpinBox>{&oscGlitches, "codecOutDmaUnderrun"},
          OscWidgetMapper<QSpinBox>{&oscGlitches, "codecInXRun"},
          OscWidgetMapper<QSpinBox>{&oscGlitches, "codecInDmaOverrun"},
      } {
	ui->setupUi(this);

	oscEnableAutoConnect.setWidget(ui->autoConnectJackClients);
	oscMonitorConnections.setWidget(ui->monitorConnections);

	oscSaveCount.setWidget(ui->saveCountSpinBox, false);
	oscSaveNow.setWidget(ui->saveNowPushButton, true);

	oscEnableMicBias.setWidget(ui->enableMicBiasCheckBox);

	// Convert Hz to Mhz
	cpuFrequency.setScale(1.f / 1000000.f);
	cpuFrequency.setWidget(ui->cpuFrequencySpinBox, false);
	cpuDivider.setValueMappingCallback([](int32_t value, bool toWidget) {
		if(toWidget)
			return (int32_t) log2(value);
		else
			return 1 << value;
	});
	cpuDivider.setWidget(ui->cpuDividerComboBox);
	cpuManualControl.setWidget(ui->cpuManualControlCheckBox);

	// Convert 1/1000000 values to percents (1/100)
	timeUsbInterrupt.setScale(1.f / 10000.f);
	timeAudioProcessing.setScale(1.f / 10000.f);
	timeOtherInterrupts.setScale(1.f / 10000.f);
	timeMainLoop.setScale(1.f / 10000.f);

	timeUsbInterrupt.setWidget(ui->usbInterruptsSpinBox, false);
	timeAudioProcessing.setWidget(ui->audioProcessingSpinBox, false);
	timeOtherInterrupts.setWidget(ui->otherInterruptsSpinBox, false);
	timeMainLoop.setWidget(ui->timersProcessingSpinBox, false);

	timeUsbInterrupt.addChangeCallback([this](float) { updateCpuTotalUsage(); });
	timeAudioProcessing.addChangeCallback([this](float) { updateCpuTotalUsage(); });
	timeOtherInterrupts.addChangeCallback([this](float) { updateCpuTotalUsage(); });
	timeMainLoop.addChangeCallback([this](float) { updateCpuTotalUsage(); });

	timePerLoopUsbInterrupt.setScale(1.f / 10.f);
	timePerLoopAudioProcessing.setScale(1.f / 10.f);
	timePerLoopOtherInterrupts.setScale(1.f / 10.f);
	timePerLoopMainLoop.setScale(1.f / 10.f);

	timePerLoopUsbInterrupt.setWidget(ui->usbInterruptsPerLoopSpinBox, false);
	timePerLoopAudioProcessing.setWidget(ui->audioProcessingPerLoopSpinBox, false);
	timePerLoopOtherInterrupts.setWidget(ui->otherInterruptsPerLoopSpinBox, false);
	timePerLoopMainLoop.setWidget(ui->timersProcessingPerLoopSpinBox, false);

	timePerLoopUsbInterrupt.addChangeCallback([this](float) { updateCpuTotalUsagePerLoop(); });
	timePerLoopAudioProcessing.addChangeCallback([this](float) { updateCpuTotalUsagePerLoop(); });
	timePerLoopOtherInterrupts.addChangeCallback([this](float) { updateCpuTotalUsagePerLoop(); });

	fastMemoryUsed.setWidget(ui->usedFastMemorySpinBox, false);
	fastMemoryUsed.addChangeCallback([this](int32_t value) { updateMemoryUsagePercent(); });

	fastMemoryAvailable.setWidget(ui->availableFastMemorySpinBox, false);
	fastMemoryAvailable.addChangeCallback([this](int32_t value) { updateMemoryUsagePercent(); });

	slowMemoryUsed.setWidget(ui->allocatedSlowMemorySpinBox, false);
	slowMemoryUsed.addChangeCallback([this](int32_t value) { updateMemoryUsagePercent(); });

	slowMemoryAvailable.setWidget(ui->availableSlowMemorySpinBox, false);
	slowMemoryAvailable.addChangeCallback([this](int32_t value) { updateMemoryUsagePercent(); });

	oscGlitchesResetCounters.setWidget(ui->glitchResetCountersPushButton, true);

	glitchesCounters[0].setWidget(ui->glitchUsbIsochronousTransferLostSpinBox, false);
	glitchesCounters[1].setWidget(ui->glitchUsbOutOverrunSpinBox, false);
	glitchesCounters[2].setWidget(ui->glitchUsbOutUnderrunSpinBox, false);
	glitchesCounters[3].setWidget(ui->glitchUsbInOverrunSpinBox, false);
	glitchesCounters[4].setWidget(ui->glitchAudioIrqLostSpinBox, false);
	glitchesCounters[5].setWidget(ui->glitchCodecOutXRunSpinBox, false);
	glitchesCounters[6].setWidget(ui->glitchCodecOutDmaUnderrunSpinBox, false);
	glitchesCounters[7].setWidget(ui->glitchCodecInXRunSpinBox, false);
	glitchesCounters[8].setWidget(ui->glitchCodecInDmaOverrunSpinBox, false);

	std::function lambdaGlitchSupported = [this]() { emit glitchDetectionSupported(); };
	std::function lambdaGlitchOccurred = [this](int32_t value) {
		if(value > 0) {
			emit glitchOccurred();
		}
	};
	for(size_t i = 0; i < glitchesCounters.size(); i++) {
		glitchesCounters[i].addOscEndpointSupportedCallback(lambdaGlitchSupported);
		glitchesCounters[i].addChangeCallback(lambdaGlitchOccurred);
	}

	manageWidgetsVisiblity();
}

GlobalConfigDialog::~GlobalConfigDialog() {
	delete ui;
}

void GlobalConfigDialog::resetGlitchCounters() {
	emit ui->glitchResetCountersPushButton->clicked();
}

void GlobalConfigDialog::showEvent(QShowEvent*) {
	emit showStateChanged(true);
}

void GlobalConfigDialog::hideEvent(QHideEvent*) {
	emit showStateChanged(false);
}

void GlobalConfigDialog::updateCpuTotalUsage() {
	float totalCpuUsage =
	    timeUsbInterrupt.get() + timeAudioProcessing.get() + timeOtherInterrupts.get() + timeMainLoop.get();
	ui->totalCpuUsageSpinBox->setValue(totalCpuUsage / 10000.f);
}

void GlobalConfigDialog::updateCpuTotalUsagePerLoop() {
	float totalCpuUsage =
	    timePerLoopUsbInterrupt.get() + timePerLoopAudioProcessing.get() + timePerLoopOtherInterrupts.get();
	ui->totalCpuUsagePerLoopSpinBox->setValue(totalCpuUsage / 10.f);
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