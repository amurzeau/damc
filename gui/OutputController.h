#ifndef OUTPUTCONTROLLER_H
#define OUTPUTCONTROLLER_H

#include "../waveSendUDPJack/Filter/EqFilter.h"
#include "WavePlayOutputInterface.h"
#include <QWidget>

namespace Ui {
class OutputController;
}

class MainWindow;
class EqualizersController;
class CompressorController;
class BalanceController;
class QwtThermo;
class LevelMeterWidget;

class OutputController : public QWidget {
	Q_OBJECT
public:
	explicit OutputController(MainWindow* parent, int numEq);
	~OutputController();

	void setInterface(int index, WavePlayInterface* interface);
	WavePlayOutputInterface* getOutputInterface() { return &interface; };

	void updateHiddenState();

protected slots:
	void onChangeVolume(int volume);
	void onChangeDelay(double delay);
	void onChangeClockDrift();
	void onEnable(int state);
	void onMute(bool muted);
	void onChangeEq(int index, bool enabled, FilterType type, double f0, double q, double gain);
	void onChangeCompressor(bool enabled,
	                        float releaseTime,
	                        float attackTime,
	                        float threshold,
	                        float makeUpGain,
	                        float compressionRatio,
	                        float kneeWidth,
	                        bool useMovingMax);
	void onChangeExpander(bool enabled,
	                      float releaseTime,
	                      float attackTime,
	                      float threshold,
	                      float makeUpGain,
	                      float compressionRatio,
	                      float kneeWidth,
	                      bool useMovingMax);
	void onChangeBalance(size_t channel, float balance);

	void onMessageReceived(const QJsonObject& message);

	void onShowEq();
	void onShowCompressor();
	void onShowExpander();
	void onShowBalance();

protected:
	void setDisplayedVolume(int volume);
	void setNumChannel(int numChannel);

	void sendChangeCompressor(const char* filterName,
	                          bool enabled,
	                          float releaseTime,
	                          float attackTime,
	                          float threshold,
	                          float makeUpGain,
	                          float ratio,
	                          float kneeWidth,
	                          bool useMovingMax);

	void dragEnterEvent(QDragEnterEvent* event) override;
	void dragMoveEvent(QDragMoveEvent* event) override;
	void dropEvent(QDropEvent* event) override;
	void mousePressEvent(QMouseEvent* event) override;

private:
	Ui::OutputController* ui;
	MainWindow* mainWindow;
	WavePlayOutputInterface interface;
	int numEq;
	int numChannels;
	EqualizersController* equalizersController;
	CompressorController* compressorController;
	CompressorController* expanderController;
	BalanceController* balanceController;
	std::vector<LevelMeterWidget*> levelWidgets;
};

#endif  // OUTPUTCONTROLLER_H
