#ifndef BODEPLOT_H
#define BODEPLOT_H

#include "../waveSendUDPJack/EqFilter.h"
#include <complex>
#include <qwt_plot.h>

class QwtPlotCurve;
class QwtPlotMarker;

class BodePlot : public QwtPlot {
	Q_OBJECT
public:
	BodePlot(QWidget* parent);

	void setNumEq(int numEq);

	QSize sizeHint() const override { return minimumSizeHint(); }

public Q_SLOTS:
	void setParameters(int index, EqFilter::FilterType filterType, double f0, double Q, double gain);

private:
	void showData(const double* frequency, const double* amplitude, const double* phase, int count);
	void showPeak(double freq, double amplitude);
	void show3dB(double freq);

	QwtPlotCurve* d_curve1;
	QwtPlotCurve* d_curve2;

	std::vector<EqFilter> eqFilters;
};

#endif  // BODEPLOT_H
