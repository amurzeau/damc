#include "OutputController.h"
#include "EqualizersController.h"
#include "ui_OutputController.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <math.h>
#include <qwt_thermo.h>

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

	connect(&interface, SIGNAL(onMessage(QJsonObject)), this, SLOT(onMessageReceived(const QJsonObject&)));
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
	QJsonObject json;
	json["volume"] = volume;
	interface.sendMessage(json);
	setDisplayedVolume(volume);
	ui->muteButton->setChecked(false);
}

void OutputController::onChangeDelay(double delay) {
	QJsonObject json;
	QJsonObject jsonDelay;
	jsonDelay["delay"] = delay;
	json["delayFilter"] = jsonDelay;
	interface.sendMessage(json);
}

void OutputController::onMute(bool muted) {
	qDebug("Muting");
	QJsonObject json;
	json["mute"] = muted;
	interface.sendMessage(json);
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

	QJsonObject json;
	QJsonArray eqFiltersArray;
	QJsonObject eqFilter;

	eqFilter["index"] = index;
	eqFilter["type"] = (int) type;
	eqFilter["f0"] = f0;
	eqFilter["Q"] = q;
	eqFilter["gain"] = gain;

	eqFiltersArray.append(eqFilter);
	json["eqFilters"] = eqFiltersArray;

	interface.sendMessage(json);
}

void OutputController::onMessageReceived(const QJsonObject& message) {
	QJsonValue levelValue = message.value("levels");
	if(levelValue.type() == QJsonValue::Array) {
		QJsonArray levels = levelValue.toArray();
		double maxLevel = -INFINITY;
		int levelNumber = std::min(levels.size(), (int) levelWidgets.size());

		for(int i = 0; i < levelNumber; i++) {
			double level = levels[i].toDouble();
			levelWidgets[i]->setValue(translateLevel(level));
			if(level > maxLevel)
				maxLevel = level;
		}
		if(maxLevel <= -192)
			ui->levelLabel->setText("--");
		else
			ui->levelLabel->setText(QString::number((int) maxLevel));
	} else {
		qDebug("Received message %s", QJsonDocument(message).toJson(QJsonDocument::Indented).constData());

		QJsonValue muteValue = message.value("mute");
		if(muteValue.type() != QJsonValue::Undefined) {
			ui->volumeSlider->blockSignals(true);
			ui->muteButton->setChecked(muteValue.toBool());
			ui->volumeSlider->blockSignals(false);
		}

		QJsonValue volumeValue = message.value("volume");
		if(volumeValue.type() != QJsonValue::Undefined) {
			int integerVolume = round(volumeValue.toDouble());
			ui->volumeSlider->blockSignals(true);
			ui->volumeSlider->setValue(integerVolume);
			ui->volumeSlider->blockSignals(false);
			setDisplayedVolume(integerVolume);
		}

		QJsonValue delayValue = message.value("delayFilter");
		if(delayValue.type() != QJsonValue::Undefined) {
			ui->delaySpinBox->blockSignals(true);
			ui->delaySpinBox->setValue(delayValue.toObject()["delay"].toDouble());
			ui->delaySpinBox->blockSignals(false);
		}

		QJsonValue eqFiltersValue = message.value("eqFilters");
		if(eqFiltersValue.type() == QJsonValue::Array) {
			QJsonArray eqFilters = eqFiltersValue.toArray();
			for(int i = 0; i < eqFilters.size(); i++) {
				QJsonObject eqFilter = eqFilters[i].toObject();

				equalizersController->setEqualizerParameters(eqFilter["index"].toInt(),
				                                             eqFilter["type"].toInt(),
				                                             eqFilter["f0"].toDouble(),
				                                             eqFilter["Q"].toDouble(),
				                                             eqFilter["gain"].toDouble());
			}
		}
	}
}

double OutputController::translateLevel(double level) {
	if(level == -INFINITY)
		return -40;
	return level;
}

void OutputController::setDisplayedVolume(int volume) {
	ui->volumeLevelLabel->setText(QString::number(volume) + " dB");
}
