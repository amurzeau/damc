#include "FastLabelWidget.h"
#include <QPainter>

FastLabelWidget::FastLabelWidget(QWidget* parent) : QWidget(parent) {}

void FastLabelWidget::setTextSize(int numCharacter, int flags) {
	QString data;

	for(int i = 0; i < numCharacter; i++)
		data.append('0');

	setMinimumWidth(fontMetrics().horizontalAdvance(data));
	setMinimumHeight(fontMetrics().height());
	setMaximumSize(minimumSize());
	setAutoFillBackground(true);
	setBackgroundRole(QPalette::Window);

	this->flags = flags;
}

void FastLabelWidget::setText(QString text) {
	if(this->text != text) {
		this->text = text;
		update();
	}
}

void FastLabelWidget::paintEvent(QPaintEvent*) {
	QPainter painter(this);
	painter.drawText(rect(), flags, text);
}
