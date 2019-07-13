#include "BalanceController.h"
#include "Balance.h"
#include "ui_BalanceController.h"
#include <QHideEvent>

BalanceController::BalanceController(QWidget* parent) : QDialog(parent), ui(new Ui::BalanceController) {
	ui->setupUi(this);
}

BalanceController::~BalanceController() {
	delete ui;
}

void BalanceController::setChannelNumber(int numChannel) {
	for(Balance* balance : balances) {
		ui->verticalLayout->removeWidget(balance);
		delete balance;
	}
	balances.clear();

	for(int i = 0; i < numChannel; i++) {
		Balance* balance = new Balance(this, i);
		ui->verticalLayout->addWidget(balance);
		connect(balance, SIGNAL(balanceChanged(size_t, float)), this, SIGNAL(parameterChanged(size_t, float)));
		balances.push_back(balance);
	}
}

void BalanceController::setParameters(size_t channel, float balance) {
	if(channel < balances.size())
		balances[channel]->setBalance(balance);
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
