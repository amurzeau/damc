#pragma once

#include "ManagedVisibilityWidget.h"
#include "OscWidgetMapper.h"
#include "WidgetAutoHidden.h"
#include <Osc/OscContainer.h>
#include <QDialog>

namespace Ui {
class CompressorController;
}

class CompressorController : public ManagedVisibilityWidget<QDialog>, public OscContainer {
public:
	explicit CompressorController(QWidget* parent, OscContainer* oscParent, const std::string& name);
	~CompressorController();

	void setEnableButton(QAbstractButton* button);

public slots:
	void show();

protected:
	virtual void hideEvent(QHideEvent* event) override;

private:
	Ui::CompressorController* ui;
	QRect savedGeometry;

	OscWidgetMapper<QAbstractButton> oscEnablePeak;
	OscWidgetMapper<QAbstractButton> oscEnableLoudness;
	OscWidgetMapper<QDoubleSpinBox> oscReleaseTime;
	OscWidgetMapper<QDoubleSpinBox> oscAttackTime;
	OscWidgetMapper<QDoubleSpinBox> oscThreshold;
	OscWidgetMapper<QDoubleSpinBox> oscStaticGain;
	OscWidgetMapper<QDoubleSpinBox> oscRatio;
	OscWidgetMapper<QDoubleSpinBox> oscKneeWidth;
	OscWidgetMapper<QDoubleSpinBox> oscLoudnessTarget;
	OscWidgetMapper<QDoubleSpinBox> oscLufsIntegrationTime;
	OscWidgetMapper<QDoubleSpinBox> oscLufsGate;
	OscWidgetMapper<QDoubleSpinBox> oscLufsMeter;
};
