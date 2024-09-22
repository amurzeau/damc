#include "CompressorController.h"
#include "ui_CompressorController.h"
#include <QHideEvent>

CompressorController::CompressorController(QWidget* parent, OscContainer* oscParent, const std::string& name)
    : ManagedVisibilityWidget<QDialog>(parent),
      OscContainer(oscParent, name),
      ui(new Ui::CompressorController),
      oscEnablePeak(this, "enable"),
      oscEnableLoudness(this, "enableLoudness"),
      oscReleaseTime(this, "releaseTime"),
      oscAttackTime(this, "attackTime"),
      oscThreshold(this, "threshold"),
      oscStaticGain(this, "makeUpGain"),
      oscRatio(this, "ratio"),
      oscKneeWidth(this, "kneeWidth"),
      oscLoudnessTarget(this, "lufsTarget"),
      oscLufsIntegrationTime(this, "lufsIntegTime"),
      oscLufsGate(this, "lufsGate"),
      oscLufsMeter(this, "lufsMeter") {
	ui->setupUi(this);

	ui->gridLayout->setAlignment(Qt::AlignHCenter);

	oscEnablePeak.setWidget(ui->enablePeakCheckBox);
	oscEnableLoudness.setWidget(ui->enableLoudnessCheckBox);
	oscReleaseTime.setWidget(ui->releaseTimeSpinBox);
	oscAttackTime.setWidget(ui->attackTimeSpinBox);
	oscThreshold.setWidget(ui->thresholdSpinBox);
	oscStaticGain.setWidget(ui->staticGainSpinBox);
	oscRatio.setWidget(ui->ratioSpinBox);
	oscKneeWidth.setWidget(ui->kneeWidthSpinBox);
	oscLoudnessTarget.setWidget(ui->loudnessTargetSpinBox);
	oscLufsIntegrationTime.setWidget(ui->lufsIntegrationTimeSpinBox);
	oscLufsGate.setWidget(ui->lufsGateSpinBox);
	oscLufsMeter.setWidget(ui->lufsMeterSpinBox);

	oscReleaseTime.setScale(1000);
	oscAttackTime.setScale(1000);
	oscLufsIntegrationTime.setScale(1000);

	addOscEnableVariable(&oscEnablePeak);
	addOscEnableVariable(&oscEnableLoudness);

	manageWidgetsVisiblity();
}

CompressorController::~CompressorController() {
	delete ui;
}

void CompressorController::show() {
	QDialog::show();
	if(savedGeometry.isValid())
		setGeometry(savedGeometry);
}

void CompressorController::hideEvent(QHideEvent*) {
	qDebug("hide event");
	savedGeometry = geometry();
}
