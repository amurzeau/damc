// From https://stackoverflow.com/questions/52550633/how-to-emit-a-signal-if-double-clicking-on-slider-handle

#include "DoubleClickSlider.h"
#include <QStyleOption>

DoubleClickSlider::DoubleClickSlider(QWidget* parent) : QSlider(parent) {}

void DoubleClickSlider::mouseDoubleClickEvent(QMouseEvent* event) {
	QStyleOptionSlider opt;
	this->initStyleOption(&opt);
	QRect sr = this->style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderHandle, this);

	if(sr.contains(event->pos())) {
		printf("Set slider to 0\n");
		setValue(0);
	} else {
		QSlider::mouseDoubleClickEvent(event);
	}
}