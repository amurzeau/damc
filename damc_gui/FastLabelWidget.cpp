#include "FastLabelWidget.h"
#include <QPainter>

FastLabelWidget::FastLabelWidget(QWidget* parent) : QWidget(parent) {}

void FastLabelWidget::setTextSize(int numCharacter, int flags) {
	QString data;

	for(int i = 0; i < numCharacter; i++)
		data.append('0');

#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
	setMinimumWidth(fontMetrics().horizontalAdvance(data));
#else
	setMinimumWidth(fontMetrics().width(data));
#endif
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
