#pragma once

#include <QWidget>

class FastLabelWidget : public QWidget {
	Q_OBJECT
public:
	explicit FastLabelWidget(QWidget* parent = nullptr);

	void setTextSize(int numCharacter, int flags);
	void setText(QString text);

	void paintEvent(QPaintEvent* event) override;

private:
	QString text;
	int flags;
};
