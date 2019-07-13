#include "BodePlotWidget.h"
#include <QPainter>
#include <QStaticText>
#include <math.h>

BodePlotWidget::BodePlotWidget(QWidget* parent) : QWidget(parent) {
	setAutoFillBackground(true);
	setBackgroundRole(QPalette::Window);
}

static void drawText(QPainter& painter,
                     qreal x,
                     qreal y,
                     Qt::Alignment flags,
                     qreal margin,
                     const QString& text,
                     QRectF* boundingRect = 0) {
	const qreal size = 1000000.0;
	QPointF corner(x, y - size);
	if(flags & Qt::AlignHCenter)
		corner.rx() -= size / 2.0;
	else if(flags & Qt::AlignRight)
		corner.rx() -= size;
	if(flags & Qt::AlignVCenter)
		corner.ry() += size / 2.0;
	else if(flags & Qt::AlignTop)
		corner.ry() += size;
	else
		flags |= Qt::AlignBottom;
	QRectF rect{corner.x() + margin, corner.y() + margin, size - margin * 2, size - margin * 2};
	painter.drawText(rect, flags, text, boundingRect);
}

void BodePlotWidget::paintEvent(QPaintEvent* event) {
	Q_UNUSED(event)

	QPainter painter(this);

	//	painter.translate(0, height());
	//	painter.scale(1, -1);
	//	painter.resetTransform();
	// painter.setWindow(0, 0, 1, 1);

	const int textMargin = 3;
	int xAxisOffset = fontMetrics().maxWidth() + textMargin;
	int yAxisOffset = fontMetrics().height() + textMargin;

	painter.setViewport(xAxisOffset, yAxisOffset, width() - 2 * xAxisOffset, height() - 2 * yAxisOffset);

	QPointF zeroPosition(yAxisOffset, xAxisOffset);

	std::vector<QLineF>
	    gridLines;  // = {QLineF(0, 0, 0, 1), QLineF(1, 0, 1, 1), QLineF(0, 0, 1, 0), QLineF(0, 1, 1, 1)};
	std::vector<std::tuple<float, float, QString, Qt::Alignment>> axisLegends;

	// Vertical lines for log scale X axis
	for(float x = powf(10, floorf(log10f(xAxis.min))); x <= xAxis.max; x *= 10) {
		int i = 1;

		while(x * i < xAxis.min)
			i++;

		for(; i < 10 && x * i < xAxis.max; i++) {
			if(x * i < xAxis.min)
				continue;
			float position = (log10(x * i) - log10(xAxis.min)) / (log10(xAxis.max) - log10(xAxis.min));
			gridLines.push_back(QLineF(position, 0, position, 1));
			if(i == 1)
				axisLegends.push_back(
				    std::make_tuple(position, 1.0f, QString::number(x * i), Qt::AlignHCenter | Qt::AlignTop));
		}
	}
	// Horizontal lines for Y axis
	for(float y = yLeftAxis.min, range = yLeftAxis.max - yLeftAxis.min; y <= yLeftAxis.max; y += range / 10) {
		float position = 1.0f - (y - yLeftAxis.min) / range;
		gridLines.push_back(QLineF(0, position, 1, position));
		if(((int) y % ((int) (range / 10))) == 0 && y != yLeftAxis.min)
			axisLegends.push_back(
			    std::make_tuple(0, position, QString::number((int) y), Qt::AlignRight | Qt::AlignVCenter));
	}
	// Horizontal lines for Y right axis
	for(float y = yRightAxis.min, range = yRightAxis.max - yRightAxis.min; y < yRightAxis.max; y += range / 10) {
		float position = 1.0f - (y - yRightAxis.min) / range;
		if(((int) y % ((int) (range / 10))) == 0 && y != yRightAxis.min)
			axisLegends.push_back(
			    std::make_tuple(1.0f, position, QString::number((int) y), Qt::AlignLeft | Qt::AlignVCenter));
	}

	// Plain borders except top
	painter.setPen(QPen(Qt::gray, 0, Qt::SolidLine));
	for(const QLineF& line : {QLineF(0, 0, 0, 1), QLineF(1, 0, 1, 1), QLineF(0, 1, 1, 1)}) {
		painter.drawLine(line.x1() * width(), line.y1() * height(), line.x2() * width(), line.y2() * height());
	}

	// Min/max on x axis
	axisLegends.push_back(std::make_tuple(0.0f, 1.0f, QString::number(xAxis.min), Qt::AlignHCenter | Qt::AlignTop));
	axisLegends.push_back(std::make_tuple(1.0f, 1.0f, QString::number(xAxis.max), Qt::AlignHCenter | Qt::AlignTop));

	// Min/max on y left axis
	axisLegends.push_back(
	    std::make_tuple(0.0f, 1.0f, QString::number(yLeftAxis.min), Qt::AlignRight | Qt::AlignVCenter));
	axisLegends.push_back(
	    std::make_tuple(0.0f, 0.0f, QString::number(yLeftAxis.max), Qt::AlignRight | Qt::AlignVCenter));

	// Min/max on y right axis
	axisLegends.push_back(
	    std::make_tuple(1.0f, 1.0f, QString::number(yRightAxis.min), Qt::AlignLeft | Qt::AlignVCenter));
	axisLegends.push_back(
	    std::make_tuple(1.0f, 0.0f, QString::number(yRightAxis.max), Qt::AlignLeft | Qt::AlignVCenter));

	// Grid lines
	painter.setPen(QPen(Qt::gray, 0, Qt::DotLine));
	// painter.scale(1.0f / width(), 1.0f / height());
	// painter.drawLines(gridLines.data(), gridLines.size());
	for(const QLineF& line : gridLines) {
		painter.drawLine(QLineF(line.x1() * width(), line.y1() * height(), line.x2() * width(), line.y2() * height()));
		// printf("Line: (%.3f %.3f) (%.3f %.3f)\n", line.x1(), line.y1(), line.x2(), line.y2());
	}

	for(const auto& text : axisLegends) {
		drawText(painter,
		         std::get<0>(text) * width(),
		         std::get<1>(text) * height(),
		         std::get<3>(text),
		         textMargin,
		         std::get<2>(text));
	}

	// Curves

	painter.setRenderHint(QPainter::Antialiasing, true);
	painter.setRenderHint(QPainter::HighQualityAntialiasing, true);

	for(PlotCurve* curve : curves) {
		float min, max;
		std::pair<double, double> previousPoint;
		bool firstPoint = true;

		switch(curve->axis) {
			case AxisType::YLeft:
				min = yLeftAxis.min;
				max = yLeftAxis.max;
				break;
			case AxisType::YRight:
				min = yRightAxis.min;
				max = yRightAxis.max;
				break;
			default:
				continue;
		}

		painter.setPen(curve->pen);

		for(const std::pair<double, double>& point : curve->points) {
			if(point.first < xAxis.min || point.first > xAxis.max)
				continue;
			std::pair<double, double> drawPosition =
			    std::make_pair((log10(point.first) - log10(xAxis.min)) / (log10(xAxis.max) - log10(xAxis.min)),
			                   (max - point.second) / (max - min));
			if(!firstPoint)
				painter.drawLine(QLineF(previousPoint.first * width(),
				                        previousPoint.second * height(),
				                        drawPosition.first * width(),
				                        drawPosition.second * height()));
			previousPoint = drawPosition;
			firstPoint = false;
		}
	}
}

void BodePlotWidget::setBackgroundBrush(QBrush brush) {
	QPalette pal = palette();
	pal.setBrush(QPalette::Window, brush);
	setPalette(pal);
}
