#include "EqualizersController.h"
#include "Equalizer.h"
#include "ui_EqualizersController.h"
#include <QHideEvent>

EqualizersController::EqualizersController(QWidget* parent, int numEq)
    : QDialog(parent), ui(new Ui::EqualizersController) {
	ui->setupUi(this);
	ui->bodePlot->setNumEq(numEq);

	for(int i = 0; i < numEq; i++) {
		Equalizer* eq = new Equalizer(this, i);
		ui->eqHorizontalLayout->addWidget(eq);
		connect(eq,
		        SIGNAL(changeParameters(int, bool, EqFilter::FilterType, double, double, double)),
		        ui->bodePlot,
		        SLOT(setParameters(int, bool, EqFilter::FilterType, double, double, double)));
		equalizers.push_back(eq);
	}
}

EqualizersController::~EqualizersController() {
	delete ui;
}

void EqualizersController::connectEqualizers(QObject* obj, const char* slot) {
	for(Equalizer* eq : equalizers) {
		connect(eq, SIGNAL(changeParameters(int, bool, EqFilter::FilterType, double, double, double)), obj, slot);
	}
}

void EqualizersController::setEqualizerParameters(int index, bool enabled, int type, double f0, double q, double gain) {
	equalizers[index]->setParameters(enabled, type, f0, q, gain);
	ui->bodePlot->setParameters(index, enabled, (FilterType) type, f0, q, gain);
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
