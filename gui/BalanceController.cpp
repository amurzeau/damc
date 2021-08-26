#include "BalanceController.h"
#include "Balance.h"
#include "ui_BalanceController.h"
#include <QHideEvent>

BalanceController::BalanceController(QWidget* parent, OscContainer* oscParent)
    : QDialog(parent), ui(new Ui::BalanceController), oscBalances(oscParent, "balance") {
	ui->setupUi(this);
	oscBalances.setWidget(this, ui->verticalLayout, [](QWidget* parent, OscContainer* oscParent, int name) -> QWidget* {
		return new Balance(parent, oscParent, std::to_string(name));
	});
}

BalanceController::~BalanceController() {
	delete ui;
}

void BalanceController::show() {
	QDialog::show();
	if(savedGeometry.isValid())
		setGeometry(savedGeometry);
}

void BalanceController::hideEvent(QHideEvent*) {
	qDebug("hide event");
	savedGeometry = geometry();
}
