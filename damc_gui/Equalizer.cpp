#include "Equalizer.h"
#include "BodePlot.h"
#include "OutputController.h"
#include "ui_Equalizer.h"

Equalizer::Equalizer(QWidget* parent,
                     OscContainer* oscParent,
                     const std::string& name,
                     OutputController* outputController,
                     BodePlot* bodePlot)
    : QWidget(parent),
      OscWidgetArray(oscParent, name),
      ui(new Ui::Equalizer),
      outputController(outputController),
      bodePlot(bodePlot),
      oscEnable(this, "enable"),
      oscType(this, "type"),
      oscF0(this, "f0"),
      oscQ(this, "Q"),
      oscGain(this, "gain") {
	ui->setupUi(this);

	bodePlot->addEqualizer(this);

	oscEnable.setWidget(ui->enableCheckBox);
	oscType.setWidget(ui->typeComboBox);
	oscF0.setWidget(ui->f0SpinBox);
	oscQ.setWidget(ui->qSpinBox);
	oscGain.setWidget(ui->gainSpinBox);

	oscEnable.addChangeCallback([this](bool) {
		updateResponse();
		this->outputController->updateEqEnable();
	});
	oscType.addChangeCallback([this](int32_t) { updateResponse(); });
	oscF0.addChangeCallback([this](float) { updateResponse(); });
	oscQ.addChangeCallback([this](float) { updateResponse(); });
	oscGain.addChangeCallback([this](float) { updateResponse(); });
}

Equalizer::~Equalizer() {
	bodePlot->removeEqualizer(this);
	delete ui;
}

std::complex<double> Equalizer::getResponse(double f0) {
	if(fs)
		return biquadFilter.getResponse(f0, fs);
	else
		return std::complex<double>(1, 0);
}

bool Equalizer::getEnabled() {
	return ui->enableCheckBox->isChecked();
}

void Equalizer::setParameters(bool enabled, FilterType filterType, double fc, double gain, double Q) {
	ui->enableCheckBox->setChecked(enabled);
	ui->typeComboBox->setCurrentIndex((int) filterType);
	ui->f0SpinBox->setValue(fc);
	ui->gainSpinBox->setValue(gain);
	ui->qSpinBox->setValue(Q);
}

void Equalizer::getParameters(bool& enabled, FilterType& filterType, double& fc, double& gain, double& Q) {
	enabled = ui->enableCheckBox->isChecked();
	filterType = (FilterType) ui->typeComboBox->currentIndex();
	fc = ui->f0SpinBox->value();
	gain = ui->gainSpinBox->value();
	Q = ui->qSpinBox->value();
}

void Equalizer::updateResponse() {
	this->fs = outputController->getSampleRate();
	biquadFilter.computeFilter(ui->enableCheckBox->isChecked(),
	                           (FilterType) ui->typeComboBox->currentIndex(),
	                           ui->f0SpinBox->value(),
	                           this->fs,
	                           ui->gainSpinBox->value(),
	                           ui->qSpinBox->value());
	bodePlot->updatePlot();
}
