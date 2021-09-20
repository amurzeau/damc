#pragma once

#include <QPen>
#include <QWidget>

class BodePlotWidget : public QWidget {
	Q_OBJECT
public:
	enum class AxisType { YLeft, YRight };
	struct PlotCurve {
		std::vector<std::pair<double, double>> points;
		QPen pen;
		AxisType axis;

		void setSamples(const double* xPoints, const double* yPoints, size_t count) {
			points.clear();
			points.reserve(count);
			for(size_t i = 0; i < count; i++) {
				points.push_back(std::make_pair(xPoints[i], yPoints[i]));
			}
		}
	};
	struct PlotAxis {
		std::string name;
		double min;
		double max;

		PlotAxis() = default;

		PlotAxis(const std::string& name, double min, double max) : name(name), min(min), max(max) {}
	};

public:
	explicit BodePlotWidget(QWidget* parent = nullptr);

	void paintEvent(QPaintEvent* event) override;

	void setBackgroundBrush(QBrush brush);
	void addCurve(PlotCurve* curve) { curves.push_back(curve); }

protected:
	PlotAxis xAxis;
	PlotAxis yLeftAxis;
	PlotAxis yRightAxis;
	std::vector<PlotCurve*> curves;
};
