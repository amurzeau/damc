#pragma once

#include "OscWidgetArray.h"
#include <QDialog>
#include <QMenu>

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

protected slots:
	void onImportAction();
	void onExportAction();

protected:
	virtual void hideEvent(QHideEvent* event) override;

private:
	Ui::EqualizersController* ui;
	QRect savedGeometry;
	OscWidgetArray oscEqualizers;

	QMenu buttonMenu;
};
