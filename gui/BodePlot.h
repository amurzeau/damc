#ifndef BODEPLOT_H
#define BODEPLOT_H

#include "../waveSendUDPJack/Filter/BiquadFilter.h"
#include "BodePlotWidget.h"
#include <complex>

class BodePlot : public BodePlotWidget {
	Q_OBJECT
public:
	BodePlot(QWidget* parent);

	void setNumEq(int numEq);

	QSize sizeHint() const override { return minimumSizeHint(); }

public Q_SLOTS:
	void setParameters(int index, bool enabled, FilterType filterType, double f0, double Q, double gain);

private:
	void showData(const double* frequency, const double* amplitude, const double* phase, int count);

	PlotCurve d_curve1;
	PlotCurve d_curve2;

	std::vector<BiquadFilter> eqFilters;
};

#endif  // BODEPLOT_H
