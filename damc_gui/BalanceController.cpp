#include "BalanceController.h"
#include "Balance.h"
#include "OutputController.h"
#include "ui_BalanceController.h"
#include <QHideEvent>

BalanceController::BalanceController(OutputController* parent, OscContainer* oscParent)
    : QDialog(parent), ui(new Ui::BalanceController), oscBalances(oscParent, "balance") {
	ui->setupUi(this);
	oscBalances.setWidget(
	    this, ui->verticalLayout, [parent](QWidget* parentWidget, OscContainer* oscParent, int name) -> QWidget* {
		    Balance* balance = new Balance(parentWidget, oscParent, std::to_string(name));
		    balance->addChangeCallback([parent](bool) { parent->updateBalanceEnable(); });
		    return balance;
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

bool BalanceController::getEnabled() {
	bool balanceEnabled = false;
	for(auto item : oscBalances.getWidgets()) {
		Balance* balance = (Balance*) item;
		if(balance->get() != 0.0f) {
			balanceEnabled = true;
			break;
		}
	}

	return balanceEnabled;
}
