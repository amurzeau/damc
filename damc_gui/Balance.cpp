#include "Balance.h"
#include "ui_Balance.h"

Balance::Balance(QWidget* parent, OscContainer* oscParent, const std::string& name)
    : QWidget(parent), OscWidgetMapper<QDoubleSpinBox>(oscParent, name), ui(new Ui::Balance) {
	ui->setupUi(this);

	size_t index = atoi(name.c_str());

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

	this->setWidget(ui->balanceValueSpinBox);
}

Balance::~Balance() {
	delete ui;
}
