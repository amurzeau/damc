#include "CompressorController.h"
#include "ui_CompressorController.h"
#include <QHideEvent>

CompressorController::CompressorController(QWidget* parent) : QDialog(parent), ui(new Ui::CompressorController) {
	ui->setupUi(this);

	ui->gridLayout->setAlignment(Qt::AlignHCenter);

	connect(ui->enableCheckBox, SIGNAL(toggled(bool)), this, SLOT(onParameterChanged()));
	connect(ui->releaseTimeSpinBox, SIGNAL(valueChanged(double)), this, SLOT(onParameterChanged()));
	connect(ui->attackTimeSpinBox, SIGNAL(valueChanged(double)), this, SLOT(onParameterChanged()));
	connect(ui->thresholdSpinBox, SIGNAL(valueChanged(double)), this, SLOT(onParameterChanged()));
	connect(ui->staticGainSpinBox, SIGNAL(valueChanged(double)), this, SLOT(onParameterChanged()));
	connect(ui->ratioSpinBox, SIGNAL(valueChanged(double)), this, SLOT(onParameterChanged()));
	connect(ui->kneeWidthSpinBox, SIGNAL(valueChanged(double)), this, SLOT(onParameterChanged()));
	connect(ui->useMovingMaxCheckBox, SIGNAL(toggled(bool)), this, SLOT(onParameterChanged()));
}

CompressorController::~CompressorController() {
	delete ui;
}

void CompressorController::setParameters(bool enabled,
                                         float releaseTime,
                                         float attackTime,
                                         float threshold,
                                         float makeUpGain,
                                         float compressionRatio,
                                         float kneeWidth,
                                         bool useMovingMax) {
	ui->enableCheckBox->setChecked(enabled);
	ui->releaseTimeSpinBox->setValue(releaseTime * 1000);
	ui->attackTimeSpinBox->setValue(attackTime * 1000);
	ui->thresholdSpinBox->setValue(threshold);
	ui->staticGainSpinBox->setValue(makeUpGain);
	ui->ratioSpinBox->setValue(compressionRatio);
	ui->kneeWidthSpinBox->setValue(kneeWidth);
	ui->useMovingMaxCheckBox->setChecked(useMovingMax);
}

void CompressorController::show() {
	QDialog::show();
	if(savedGeometry.isValid())
		setGeometry(savedGeometry);
}

void CompressorController::onParameterChanged() {
	emit parameterChanged(ui->enableCheckBox->isChecked(),
	                      ui->releaseTimeSpinBox->value() / 1000.0f,
	                      ui->attackTimeSpinBox->value() / 1000.0f,
	                      ui->thresholdSpinBox->value(),
	                      ui->staticGainSpinBox->value(),
	                      ui->ratioSpinBox->value(),
	                      ui->kneeWidthSpinBox->value(),
	                      ui->useMovingMaxCheckBox->isChecked());
}

void CompressorController::hideEvent(QHideEvent*) {
	qDebug("hide event");
	savedGeometry = geometry();
}
