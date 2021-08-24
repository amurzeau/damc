#include "CompressorController.h"
#include "ui_CompressorController.h"
#include <QHideEvent>

CompressorController::CompressorController(QWidget* parent, OscContainer* oscParent, const std::string& name)
    : QDialog(parent),
      OscContainer(oscParent, name),
      ui(new Ui::CompressorController),
      oscEnable(this, "enable"),
      oscReleaseTime(this, "releaseTime"),
      oscAttackTime(this, "attackTime"),
      oscThreshold(this, "threshold"),
      oscStaticGain(this, "makeUpGain"),
      oscRatio(this, "ratio"),
      oscKneeWidth(this, "kneeWidth"),
      oscMovingMax(this, "useMovingMax") {
	ui->setupUi(this);

	ui->gridLayout->setAlignment(Qt::AlignHCenter);

	oscEnable.setWidget(ui->enableCheckBox);
	oscReleaseTime.setWidget(ui->releaseTimeSpinBox);
	oscAttackTime.setWidget(ui->attackTimeSpinBox);
	oscThreshold.setWidget(ui->thresholdSpinBox);
	oscStaticGain.setWidget(ui->staticGainSpinBox);
	oscRatio.setWidget(ui->ratioSpinBox);
	oscKneeWidth.setWidget(ui->kneeWidthSpinBox);
	oscMovingMax.setWidget(ui->useMovingMaxCheckBox);
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
