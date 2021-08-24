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

	oscEnable.setWidget(ui->parametricEqGroupBox);
	oscType.setWidget(ui->typeComboBox);
	oscF0.setWidget(ui->f0SpinBox);
	oscQ.setWidget(ui->qSpinBox);
	oscGain.setWidget(ui->gainSpinBox);

	oscEnable.setChangeCallback([this](float) { updateResponse(); });
	oscType.setChangeCallback([this](float) { updateResponse(); });
	oscF0.setChangeCallback([this](float) { updateResponse(); });
	oscQ.setChangeCallback([this](float) { updateResponse(); });
	oscGain.setChangeCallback([this](float) { updateResponse(); });
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

void Equalizer::updateResponse() {
	this->fs = outputController->getSampleRate();
	biquadFilter.computeFilter(ui->parametricEqGroupBox->isChecked(),
	                           (FilterType) ui->typeComboBox->currentIndex(),
	                           ui->f0SpinBox->value(),
	                           this->fs,
	                           ui->gainSpinBox->value(),
	                           ui->qSpinBox->value());
	bodePlot->updatePlot();
}
