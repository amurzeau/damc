#include "BodePlot.h"
#include <complex>
#include <math.h>
#include <qwt_legend.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_grid.h>
#include <qwt_plot_marker.h>
#include <qwt_scale_engine.h>
#include <qwt_symbol.h>

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
		array[i] = qExp(lxmin + double(i) * lstep);
}

BodePlot::BodePlot(QWidget* parent) : QwtPlot(parent) {
	setAutoReplot(false);

	for(EqFilter& eqFilter : eqFilters)
		eqFilter.init(EqFilter::FilterType::None, 1000, 48000, 0, 0);

	setCanvasBackground(QColor(Qt::darkBlue));

	// legend
	QwtLegend* legend = new QwtLegend;
	insertLegend(legend, QwtPlot::BottomLegend);

	// grid
	QwtPlotGrid* grid = new QwtPlotGrid;
	grid->enableXMin(true);
	grid->setMajorPen(QPen(Qt::white, 0, Qt::DotLine));
	grid->setMinorPen(QPen(Qt::gray, 0, Qt::DotLine));
	grid->attach(this);

	// axes
	enableAxis(QwtPlot::yRight);
	setAxisTitle(QwtPlot::xBottom, "Frequency");
	setAxisTitle(QwtPlot::yLeft, "Amplitude [dB]");
	setAxisTitle(QwtPlot::yRight, "Phase [deg]");

	setAxisMaxMajor(QwtPlot::xBottom, 6);
	setAxisMaxMinor(QwtPlot::xBottom, 10);
	setAxisScaleEngine(QwtPlot::xBottom, new QwtLogScaleEngine(10));
	setAxisScale(QwtPlot::xBottom, 10, 24000);
	setAxisScale(QwtPlot::yLeft, -30, +30);
	setAxisScale(QwtPlot::yRight, -180, +180);

	// curves
	d_curve1 = new QwtPlotCurve("Amplitude");
	d_curve1->setRenderHint(QwtPlotItem::RenderAntialiased);
	d_curve1->setPen(QPen(Qt::yellow));
	d_curve1->setLegendAttribute(QwtPlotCurve::LegendShowLine);
	d_curve1->setYAxis(QwtPlot::yLeft);
	d_curve1->attach(this);

	d_curve2 = new QwtPlotCurve("Phase");
	d_curve2->setRenderHint(QwtPlotItem::RenderAntialiased);
	d_curve2->setPen(QPen(Qt::cyan));
	d_curve2->setLegendAttribute(QwtPlotCurve::LegendShowLine);
	d_curve2->setYAxis(QwtPlot::yRight);
	d_curve2->attach(this);

	setAutoReplot(true);
}

void BodePlot::setNumEq(int numEq) {
	eqFilters.resize(numEq);
}

void BodePlot::showData(const double* frequency, const double* amplitude, const double* phase, int count) {
	d_curve1->setSamples(frequency, amplitude, count);
	d_curve2->setSamples(frequency, phase, count);
}

//
// re-calculate frequency response
//
void BodePlot::setParameters(int index, EqFilter::FilterType filterType, double f0, double Q, double gain) {
	const bool doReplot = autoReplot();
	setAutoReplot(false);

	const int ArraySize = 2000;

	double frequency[ArraySize];
	double amplitude[ArraySize];
	double phase[ArraySize];

	eqFilters[index].setParameters(filterType, f0, gain, Q);

	// build frequency vector with logarithmic division
	logSpace(frequency, ArraySize, 10, 24000);

	for(int i = 0; i < ArraySize; i++) {
		const double f = frequency[i];
		std::complex<double> g(1, 0);

		for(EqFilter& eqFilter : eqFilters)
			g *= eqFilter.getResponse(f);

		amplitude[i] = 20.0 * log10(qSqrt(g.real() * g.real() + g.imag() * g.imag()));
		phase[i] = qAtan2(g.imag(), g.real()) * (180.0 / M_PI);
	}

	showData(frequency, amplitude, phase, ArraySize);

	setAutoReplot(doReplot);

	replot();
}
