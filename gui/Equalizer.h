#ifndef EQUALIZER_H
#define EQUALIZER_H

#include "../waveSendUDPJack/Filter/EqFilter.h"
#include <QWidget>

namespace Ui {
class Equalizer;
}

class EqualizersController;

class Equalizer : public QWidget {
	Q_OBJECT

public:
	explicit Equalizer(QWidget* parent, int index);
	~Equalizer();
	void setParameters(bool enabled, int type, double f0, double q, double gain);

signals:
	void changeParameters(int index, bool enabled, FilterType type, double f0, double q, double gain);

private slots:
	void onParameterChanged();

private:
	Ui::Equalizer* ui;
	int index;
};

#endif  // EQUALIZER_H
