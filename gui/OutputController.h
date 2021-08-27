#ifndef OUTPUTCONTROLLER_H
#define OUTPUTCONTROLLER_H

#include "BiquadFilter.h"
#include "OscWidgetMapper.h"
#include <Osc/OscContainer.h>
#include <Osc/OscEndpoint.h>
#include <Osc/OscVariable.h>
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

class OutputController : public QWidget, public OscContainer {
	Q_OBJECT
public:
	explicit OutputController(MainWindow* parent, OscContainer* oscParent, const std::string& name);
	~OutputController();

	void updateHiddenState();
	void updateTooltip();
	void updateEqEnable();

	int32_t getSampleRate() { return oscSampleRate.get(); }

protected slots:
	void onChangeClockDrift();

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
	int numChannels;
	EqualizersController* equalizersController;
	CompressorController* compressorController;
	CompressorController* expanderController;
	BalanceController* balanceController;
	std::vector<LevelMeterWidget*> levelWidgets;

	OscContainer oscFilterChain;

	OscEndpoint oscMeterPerChannel;
	OscWidgetMapper<QAbstractButton> oscEnable;
	OscWidgetMapper<QAbstractButton> oscMute;
	OscWidgetMapper<QDoubleSpinBox> oscDelay;
	OscWidgetMapper<QDoubleSpinBox> oscClockDrift;
	OscWidgetMapper<QAbstractSlider> oscVolume;
	OscVariable<std::string> oscName;
	OscVariable<std::string> oscDisplayName;
	OscVariable<int32_t> oscSampleRate;
};

#endif  // OUTPUTCONTROLLER_H
