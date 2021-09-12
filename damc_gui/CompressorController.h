#ifndef COMPRESSORCONTROLLER_H
#define COMPRESSORCONTROLLER_H

#include "OscWidgetMapper.h"
#include <Osc/OscContainer.h>
#include <QDialog>

namespace Ui {
class CompressorController;
}

class CompressorController : public QDialog, public OscContainer {
public:
	explicit CompressorController(QWidget* parent, OscContainer* oscParent, const std::string& name);
	~CompressorController();

	OscWidgetMapper<QAbstractButton>& getEnableMapper() { return oscEnable; }

public slots:
	void show();

protected:
	virtual void hideEvent(QHideEvent* event) override;

private:
	Ui::CompressorController* ui;
	QRect savedGeometry;

	OscWidgetMapper<QAbstractButton> oscEnable;
	OscWidgetMapper<QDoubleSpinBox> oscReleaseTime;
	OscWidgetMapper<QDoubleSpinBox> oscAttackTime;
	OscWidgetMapper<QDoubleSpinBox> oscThreshold;
	OscWidgetMapper<QDoubleSpinBox> oscStaticGain;
	OscWidgetMapper<QDoubleSpinBox> oscRatio;
	OscWidgetMapper<QDoubleSpinBox> oscKneeWidth;
	OscWidgetMapper<QAbstractButton> oscMovingMax;
};

#endif  // COMPRESSORCONTROLLER_H
