#include "Reverb.h"
#include "ui_Reverb.h"
#include <QHideEvent>

Reverb::Reverb(QWidget* parent) : QDialog(parent), ui(new Ui::Reverb) {
	ui->setupUi(this);
}

Reverb::~Reverb() {
	delete ui;
}

void Reverb::show() {
	QDialog::show();
	if(savedGeometry.isValid())
		setGeometry(savedGeometry);
}

void Reverb::hideEvent(QHideEvent*) {
	savedGeometry = geometry();
}
