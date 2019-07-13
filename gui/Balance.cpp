#include "Balance.h"
#include "ui_Balance.h"

Balance::Balance(QWidget* parent, size_t index) : QWidget(parent), ui(new Ui::Balance), index(index) {
	ui->setupUi(this);

	const char* channelNames[] = {"Front Left",
	                              "Front Right",
	                              "Front Center",
	                              "LFE",
	                              "Rear Left",
	                              "Rear Right",
	                              "Alternative Rear Left",
	                              "Alternative Rear Right"};

	if(index < sizeof(channelNames) / sizeof(channelNames[0])) {
		ui->balanceNameLabel->setText(channelNames[index]);
	} else {
		ui->balanceNameLabel->setText(QString("Channel #%1").arg(index + 1));
	}

	connect(ui->balanceValueSpinBox, SIGNAL(valueChanged(double)), this, SLOT(onBalanceChanged()));
}

Balance::~Balance() {
	delete ui;
}

void Balance::setBalance(float volume) {
	ui->balanceValueSpinBox->blockSignals(true);
	ui->balanceValueSpinBox->setValue(volume);
	ui->balanceValueSpinBox->blockSignals(false);
}

void Balance::onBalanceChanged() {
	emit balanceChanged(index, ui->balanceValueSpinBox->value());
}
