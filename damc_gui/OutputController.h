#pragma once

#include "BiquadFilter.h"
#include "OscWidgetMapper.h"
#include "OutputInstanceConfigDialog.h"
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

	void showConfigDialog();
	void updateEqEnable();
	OscWidgetMapper<QAbstractButton>* getOscEnable() { return &oscEnable; }
	OscWidgetMapper<QLineEdit>* getOscName() { return &oscName; }
	OscWidgetMapper<QLineEdit>* getOscDisplayName() { return &oscDisplayName; }
	OscWidgetMapper<QSpinBox>* getOscJackSampleRate() { return &oscJackSampleRate; }

	int32_t getSampleRate() { return oscSampleRate.get(); }

protected slots:
	void onShowEq();
	void onShowCompressor();
	void onShowExpander();
	void onShowBalance();
	void onShowConfig();
	void onRemove();

protected:
	void setNumChannel(int numChannel);

	void updateHiddenState();
	void updateTooltip();

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
	OutputInstanceConfigDialog* configDialog;
	std::vector<LevelMeterWidget*> levelWidgets;

	OscContainer oscFilterChain;

	OscEndpoint oscMeterPerChannel;
	OscWidgetMapper<QAbstractButton> oscEnable;
	OscWidgetMapper<QAbstractButton> oscMute;
	OscWidgetMapper<QDoubleSpinBox> oscDelay;
	OscWidgetMapper<QAbstractSlider> oscVolume;

	OscWidgetMapper<QLineEdit> oscName;
	OscWidgetMapper<QLineEdit> oscDisplayName;

	OscVariable<int32_t> oscSampleRate;
	OscWidgetMapper<QSpinBox> oscJackSampleRate;
};
