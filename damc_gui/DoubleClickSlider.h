#pragma once

#include <QMouseEvent>
#include <QSlider>

class DoubleClickSlider : public QSlider {
	Q_OBJECT
public:
	DoubleClickSlider(QWidget* parent = nullptr);

protected:
	void mouseDoubleClickEvent(QMouseEvent* event) override;
};