#include "BodePlot.h"
#include <complex>
#include <math.h>

static void logSpace(double* array, int size, double xmin, double xmax) {
	if((xmin <= 0.0) || (xmax <= 0.0) || (size <= 0))
		return;

	const int imax = size - 1;

	array[0] = xmin;
	array[imax] = xmax;

	const double lxmin = log(xmin);
	const double lxmax = log(xmax);
	const double lstep = (lxmax - lxmin) / double(imax);

	for(int i = 1; i < imax; i++)
		array[i] = exp(lxmin + double(i) * lstep);
}

BodePlot::BodePlot(QWidget* parent) : BodePlotWidget(parent) {
	setBackgroundBrush(Qt::darkBlue);

	xAxis = PlotAxis("Frequency", 20.0f, 24000.0f);
	yLeftAxis = PlotAxis("Amplitude [dB]", -30.0f, 30.0f);
	yRightAxis = PlotAxis("Phase [deg]", -180.0f, 180.0f);

	d_curve1.pen = QPen(Qt::yellow, 0, Qt::SolidLine);
	d_curve1.axis = AxisType::YLeft;
	addCurve(&d_curve1);

	d_curve2.pen = QPen(Qt::cyan, 0, Qt::SolidLine);
	d_curve2.axis = AxisType::YRight;
	addCurve(&d_curve2);
}

void BodePlot::setNumEq(int numEq) {
	eqFilters.resize(numEq);
}

void BodePlot::showData(const double* frequency, const double* amplitude, const double* phase, int count) {
	d_curve1.setSamples(frequency, amplitude, count);
	d_curve2.setSamples(frequency, phase, count);
	update();
}

//
// re-calculate frequency response
//
void BodePlot::setParameters(int index, bool enabled, FilterType filterType, double f0, double Q, double gain) {
	static constexpr int ArraySize = 2000;
	static constexpr float SAMPLE_RATE = 48000;

	double frequency[ArraySize] = {0};
	double amplitude[ArraySize];
	double phase[ArraySize];

	eqFilters[index].computeFilter(enabled, filterType, f0, SAMPLE_RATE, gain, Q);

	// build frequency vector with logarithmic division
	logSpace(frequency, ArraySize, 20, 24000);

	for(int i = 0; i < ArraySize; i++) {
		const double f = frequency[i];
		std::complex<double> g(1, 0);

		for(BiquadFilter& eqFilter : eqFilters)
			g *= eqFilter.getResponse(f, SAMPLE_RATE);

		amplitude[i] = 20.0 * log10(sqrt(g.real() * g.real() + g.imag() * g.imag()));
		phase[i] = atan2(g.imag(), g.real()) * (180.0 / M_PI);
	}

	showData(frequency, amplitude, phase, ArraySize);

	update();
}
