#include "OutputController.h"
#include "EqualizersController.h"
#include "ui_OutputController.h"
#include <math.h>
#include <qwt/qwt_thermo.h>

OutputController::OutputController(QWidget* parent, int numEq, int numChannels)
    : QWidget(parent), ui(new Ui::OutputController) {
	ui->setupUi(this);

	for(int i = 0; i < numChannels; i++) {
		QwtThermo* level = new QwtThermo(this);
		level->setObjectName(QStringLiteral("level") + QString::number(i + 1));
		level->setLowerBound(-40);
		level->setUpperBound(6);
		level->setScalePosition(i == 0 ? QwtThermo::TrailingScale : QwtThermo::NoScale);
		level->setAlarmEnabled(true);
		level->setSpacing(0);
		level->setBorderWidth(0);
		level->setPipeWidth(3);
		level->setScaleMaxMinor(10);
		level->setScaleMaxMajor(0);
		level->setValue(translateLevel(-INFINITY));
		levelWidgets.push_back(level);
		ui->levelContainerLayout->addWidget(level);
	}

	equalizersController = new EqualizersController(this, numEq);
	equalizersController->connectEqualizers(this, SLOT(onChangeEq(int, EqFilter::FilterType, double, double, double)));

	connect(ui->volumeSlider, SIGNAL(valueChanged(int)), this, SLOT(onChangeVolume(int)));
	connect(ui->delaySpinBox, SIGNAL(valueChanged(double)), this, SLOT(onChangeDelay(double)));
	connect(ui->muteButton, SIGNAL(toggled(bool)), this, SLOT(onMute(bool)));
	connect(ui->eqButton, SIGNAL(clicked(bool)), this, SLOT(onShowEq()));

	connect(
	    &interface, SIGNAL(onMessage(notification_message_t)), this, SLOT(onMessageReceived(notification_message_t)));
}

OutputController::~OutputController() {
	delete ui;
}

void OutputController::setInterface(int index, WavePlayInterface* interface) {
	this->interface.setInterface(index, interface);

	equalizersController->setWindowTitle(tr("Equalizer parameters for output #%1").arg(index + 1));
}

void OutputController::onChangeVolume(int volume) {
	qDebug("Changing volume");
	struct control_message_t message;
	message.opcode = control_message_t::op_set_volume;
	message.data.set_volume.volume = volume;
	interface.sendMessage(message);
	setDisplayedVolume(volume);
	ui->muteButton->setChecked(false);
}

void OutputController::onChangeDelay(double delay) {
	struct control_message_t message;
	message.opcode = control_message_t::op_set_delay;
	message.data.set_delay.delay = delay / 1000.0;
	interface.sendMessage(message);
}

void OutputController::onMute(bool muted) {
	qDebug("Muting");
	struct control_message_t message;
	message.opcode = control_message_t::op_set_mute;
	message.data.set_mute.mute = muted;
	interface.sendMessage(message);
}

void OutputController::onShowEq() {
	if(equalizersController->isHidden()) {
		equalizersController->show();
	} else {
		equalizersController->hide();
	}
}

void OutputController::onChangeEq(int index, EqFilter::FilterType type, double f0, double q, double gain) {
	qDebug("Changing eq %d", index);
	struct control_message_t message;
	message.opcode = control_message_t::op_set_eq;
	message.data.set_eq.index = index;
	message.data.set_eq.type = (int) type;
	message.data.set_eq.f0 = f0;
	message.data.set_eq.Q = q;
	message.data.set_eq.gain = gain;
	interface.sendMessage(message);
}

void OutputController::onMessageReceived(notification_message_t message) {
	switch(message.opcode) {
		case notification_message_t::op_state:
			// Handled in MainWindow
			break;
		case notification_message_t::op_level:
			for(size_t i = 0; i < std::min(sizeof(message.data.level.level) / sizeof(message.data.level.level[0]),
			                               levelWidgets.size());
			    i++) {
				levelWidgets[i]->setValue(translateLevel(message.data.level.level[i]));
			}
			ui->levelLabel->setText(
			    QString::number((int) std::max(message.data.level.level[0], message.data.level.level[1])));
			break;
		case notification_message_t::op_control:
			switch(message.data.control.opcode) {
				case control_message_t::op_set_mute:
					ui->volumeSlider->blockSignals(true);
					ui->muteButton->setChecked(message.data.control.data.set_mute.mute);
					ui->volumeSlider->blockSignals(false);
					break;
				case control_message_t::op_set_volume:
					ui->volumeSlider->blockSignals(true);
					ui->volumeSlider->setValue(message.data.control.data.set_volume.volume);
					ui->volumeSlider->blockSignals(false);
					setDisplayedVolume(message.data.control.data.set_volume.volume);
					break;
				case control_message_t::op_set_delay:
					ui->delaySpinBox->blockSignals(true);
					ui->delaySpinBox->setValue(message.data.control.data.set_delay.delay * 1000.0);
					ui->delaySpinBox->blockSignals(false);
					break;
				case control_message_t::op_set_eq:
					equalizersController->setEqualizerParameters(message.data.control.data.set_eq.index,
					                                             message.data.control.data.set_eq.type,
					                                             message.data.control.data.set_eq.f0,
					                                             message.data.control.data.set_eq.Q,
					                                             message.data.control.data.set_eq.gain);
					break;
			}
			break;
	}
}

double OutputController::translateLevel(double level) {
	if(level == -INFINITY)
		return -40;
	return level;
}

void OutputController::setDisplayedVolume(double volume) {
	ui->volumeLevelLabel->setText(QString::number((int) volume) + " dB");
}
