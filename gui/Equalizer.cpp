#include "Equalizer.h"
#include "ui_Equalizer.h"

Equalizer::Equalizer(QWidget* parent, int index) : QWidget(parent), ui(new Ui::Equalizer), index(index) {
	ui->setupUi(this);

	connect(ui->typeComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(onParameterChanged()));
	connect(ui->f0SpinBox, SIGNAL(valueChanged(double)), this, SLOT(onParameterChanged()));
	connect(ui->qSpinBox, SIGNAL(valueChanged(double)), this, SLOT(onParameterChanged()));
	connect(ui->gainSpinBox, SIGNAL(valueChanged(double)), this, SLOT(onParameterChanged()));

	ui->parametricEqGroupBox->setTitle("EQ " + QString::number(index + 1));
}

Equalizer::~Equalizer() {
	delete ui;
}

void Equalizer::setParameters(int type, double f0, double q, double gain) {
	ui->typeComboBox->blockSignals(true);
	ui->f0SpinBox->blockSignals(true);
	ui->qSpinBox->blockSignals(true);
	ui->gainSpinBox->blockSignals(true);

	ui->typeComboBox->setCurrentIndex(type);
	ui->f0SpinBox->setValue(f0);
	ui->qSpinBox->setValue(q);
	ui->gainSpinBox->setValue(gain);

	ui->typeComboBox->blockSignals(false);
	ui->f0SpinBox->blockSignals(false);
	ui->qSpinBox->blockSignals(false);
	ui->gainSpinBox->blockSignals(false);
}

void Equalizer::onParameterChanged() {
	emit changeParameters(index,
	                      EqFilter::FilterType(ui->typeComboBox->currentIndex()),
	                      ui->f0SpinBox->value(),
	                      ui->qSpinBox->value(),
	                      ui->gainSpinBox->value());
}
