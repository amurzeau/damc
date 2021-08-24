#include "Equalizer.h"
#include "ui_Equalizer.h"

Equalizer::Equalizer(QWidget* parent, int index) : QWidget(parent), ui(new Ui::Equalizer), index(index) {
	ui->setupUi(this);

	connect(ui->parametricEqGroupBox, &QGroupBox::clicked, this, &Equalizer::onParameterChanged);
	connect(ui->typeComboBox, qOverload<int>(&QComboBox::currentIndexChanged), this, &Equalizer::onParameterChanged);
	connect(ui->f0SpinBox, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &Equalizer::onParameterChanged);
	connect(ui->qSpinBox, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &Equalizer::onParameterChanged);
	connect(ui->gainSpinBox, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &Equalizer::onParameterChanged);

	ui->parametricEqGroupBox->setTitle("EQ " + QString::number(index + 1));
}

Equalizer::~Equalizer() {
	delete ui;
}

void Equalizer::setParameters(bool enabled, int type, double f0, double q, double gain) {
	ui->typeComboBox->blockSignals(true);
	ui->f0SpinBox->blockSignals(true);
	ui->qSpinBox->blockSignals(true);
	ui->gainSpinBox->blockSignals(true);
	ui->parametricEqGroupBox->blockSignals(true);

	ui->parametricEqGroupBox->setChecked(true);
	ui->typeComboBox->setCurrentIndex(type);
	ui->f0SpinBox->setValue(f0);
	ui->qSpinBox->setValue(q);
	ui->gainSpinBox->setValue(gain);
	ui->parametricEqGroupBox->setChecked(enabled);

	ui->typeComboBox->blockSignals(false);
	ui->f0SpinBox->blockSignals(false);
	ui->qSpinBox->blockSignals(false);
	ui->gainSpinBox->blockSignals(false);
	ui->parametricEqGroupBox->blockSignals(false);
}

void Equalizer::onParameterChanged() {
	emit changeParameters(index,
	                      ui->parametricEqGroupBox->isChecked(),
	                      FilterType(ui->typeComboBox->currentIndex()),
	                      ui->f0SpinBox->value(),
	                      ui->qSpinBox->value(),
	                      ui->gainSpinBox->value());
}
