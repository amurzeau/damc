#ifndef BODEPLOT_H
#define BODEPLOT_H

#include "BiquadFilter.h"
#include "BodePlotWidget.h"
#include <complex>
#include <set>

class Equalizer;

class BodePlot : public BodePlotWidget {
	Q_OBJECT
public:
	BodePlot(QWidget* parent);

	void addEqualizer(Equalizer* eq);
	void removeEqualizer(Equalizer* eq);
	void updatePlot();

	QSize sizeHint() const override { return minimumSizeHint(); }

private:
	void showData(const double* frequency, const double* amplitude, const double* phase, int count);

	PlotCurve d_curve1;
	PlotCurve d_curve2;

	std::set<Equalizer*> eqFilters;
};

#endif  // BODEPLOT_H
