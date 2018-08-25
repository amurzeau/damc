#ifndef OUTPUTCONTROLLER_H
#define OUTPUTCONTROLLER_H

#include "../EqFilter.h"
#include "WavePlayOutputInterface.h"
#include <QWidget>

namespace Ui {
class OutputController;
}

class EqualizersController;
class QwtThermo;

class OutputController : public QWidget {
	Q_OBJECT
public:
	explicit OutputController(QWidget* parent, int numEq, int numChannels);
	~OutputController();

	void setInterface(int index, WavePlayInterface* interface);

protected slots:
	void onChangeVolume(int volume);
	void onChangeDelay(double delay);
	void onMute(bool muted);
	void onShowEq();
	void onChangeEq(int index, EqFilter::FilterType type, double f0, double q, double gain);
	void onChangeDithering();

	void onMessageReceived(notification_message_t message);

protected:
	double translateLevel(double level);
	void setDisplayedVolume(double volume);

private:
	Ui::OutputController* ui;
	WavePlayOutputInterface interface;
	int numEq;
	EqualizersController* equalizersController;
	std::vector<QwtThermo*> levelWidgets;
};

#endif  // OUTPUTCONTROLLER_H
