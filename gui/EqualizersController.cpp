#include "EqualizersController.h"
#include "Equalizer.h"
#include "OutputController.h"
#include "ui_EqualizersController.h"
#include <QHideEvent>

EqualizersController::EqualizersController(OutputController* parent, OscContainer* oscParent)
    : QDialog(parent), ui(new Ui::EqualizersController), oscEqualizers(oscParent, "eqFilters") {
	ui->setupUi(this);
	oscEqualizers.setWidget(
	    this,
	    ui->eqHorizontalLayout,
	    [this, parent](QWidget* widgetParent, OscContainer* oscParent, const std::string& name) -> QWidget* {
		    return new Equalizer(this, oscParent, name, parent, ui->bodePlot);
	    });
}

EqualizersController::~EqualizersController() {
	delete ui;
}

bool EqualizersController::getEqEnabled() {
	bool eqEnabled = false;
	for(auto item : oscEqualizers.getWidgets()) {
		Equalizer* eq = (Equalizer*) item;
		if(eq->getEnabled()) {
			eqEnabled = true;
			break;
		}
	}
	return eqEnabled;
}

void EqualizersController::show() {
	QDialog::show();
	if(savedGeometry.isValid())
		setGeometry(savedGeometry);
}

void EqualizersController::hideEvent(QHideEvent*) {
	qDebug("hide event");
	savedGeometry = geometry();
}
