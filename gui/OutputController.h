#ifndef OUTPUTCONTROLLER_H
#define OUTPUTCONTROLLER_H

#include "../waveSendUDPJack/Filter/EqFilter.h"
#include "WavePlayOutputInterface.h"
#include <QWidget>

namespace Ui {
class OutputController;
}

class EqualizersController;
class CompressorController;
class BalanceController;
class QwtThermo;
class LevelMeterWidget;

class OutputController : public QWidget {
	Q_OBJECT
public:
	explicit OutputController(QWidget* parent, int numEq);
	~OutputController();

	void setInterface(int index, WavePlayInterface* interface);

protected slots:
	void onChangeVolume(int volume);
	void onChangeDelay(double delay);
	void onChangeClockDrift();
	void onEnable(int state);
	void onMute(bool muted);
	void onChangeEq(int index, EqFilter::FilterType type, double f0, double q, double gain);
	void onChangeCompressor(bool enabled,
	                        float releaseTime,
	                        float attackTime,
	                        float threshold,
	                        float makeUpGain,
	                        float compressionRatio,
	                        float kneeWidth);
	void onChangeBalance(size_t channel, float balance);

	void onMessageReceived(const QJsonObject& message);

	void onShowEq();
	void onShowCompressor();
	void onShowBalance();

protected:
	double translateLevel(double level);
	void setDisplayedVolume(int volume);
	void setNumChannel(int numChannel);

private:
	Ui::OutputController* ui;
	WavePlayOutputInterface interface;
	int numEq;
	int numChannels;
	EqualizersController* equalizersController;
	CompressorController* compressorController;
	BalanceController* balanceController;
	std::vector<LevelMeterWidget*> levelWidgets;
};

#endif  // OUTPUTCONTROLLER_H
