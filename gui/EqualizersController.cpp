#include "EqualizersController.h"
#include "Equalizer.h"
#include "OutputController.h"
#include "ui_EqualizersController.h"
#include <QHideEvent>

EqualizersController::EqualizersController(OutputController* parent, OscContainer* oscParent, int numEq)
    : QDialog(parent), ui(new Ui::EqualizersController), oscEqualizers(oscParent, "eqFilters") {
	ui->setupUi(this);
	oscEqualizers.setWidget(
	    this,
	    ui->eqHorizontalLayout,
	    [this, parent](QWidget* widgetParent, OscContainer* oscParent, const std::string& name) -> QWidget* {
		    return new Equalizer(widgetParent, oscParent, name, parent, ui->bodePlot);
	    });
}

EqualizersController::~EqualizersController() {
	delete ui;
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
