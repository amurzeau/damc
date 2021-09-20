#pragma once

#include "OscWidgetArray.h"
#include <QDialog>

namespace Ui {
class EqualizersController;
}

class Equalizer;
class OutputController;

class EqualizersController : public QDialog {
	Q_OBJECT

public:
	explicit EqualizersController(OutputController* parent, OscContainer* oscParent);
	~EqualizersController();

	bool getEqEnabled();

	void show();

protected:
	virtual void hideEvent(QHideEvent* event) override;

private:
	Ui::EqualizersController* ui;
	QRect savedGeometry;
	OscWidgetArray oscEqualizers;
};
