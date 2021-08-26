#ifndef EQUALIZER_H
#define EQUALIZER_H

#include "OscWidgetArray.h"
#include "OscWidgetMapper.h"
#include <BiquadFilter.h>
#include <QWidget>

namespace Ui {
class Equalizer;
}

class EqualizersController;
class OutputController;
class BodePlot;

class Equalizer : public QWidget, public OscWidgetArray {
public:
	explicit Equalizer(QWidget* parent,
	                   OscContainer* oscParent,
	                   const std::string& name,
	                   OutputController* outputController,
	                   BodePlot* bodePlot);
	~Equalizer();

	std::complex<double> getResponse(double f0);
	bool getEnabled();

protected:
	void updateResponse();

private:
	Ui::Equalizer* ui;

	OutputController* outputController;
	BiquadFilter biquadFilter;
	BodePlot* bodePlot;

	OscWidgetMapper<QGroupBox> oscEnable;
	OscWidgetMapper<QComboBox> oscType;
	OscWidgetMapper<QDoubleSpinBox> oscF0;
	OscWidgetMapper<QDoubleSpinBox> oscQ;
	OscWidgetMapper<QDoubleSpinBox> oscGain;

	double fs;
};

#endif  // EQUALIZER_H
