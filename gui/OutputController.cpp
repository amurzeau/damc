#include "OutputController.h"
#include "BalanceController.h"
#include "CompressorController.h"
#include "EqualizersController.h"
#include "LevelMeterWidget.h"
#include "ui_OutputController.h"
#include <MainWindow.h>
#include <QDrag>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMimeData>
#include <QMouseEvent>
#include <math.h>

#define DRAG_FORMAT "application/x-dndwaveoutputinstancewidget"

OutputController::OutputController(MainWindow* parent, int numEq)
    : QWidget(parent), ui(new Ui::OutputController), mainWindow(parent) {
	ui->setupUi(this);

	numChannels = 0;

	equalizersController = new EqualizersController(this, numEq);
	equalizersController->connectEqualizers(this, SLOT(onChangeEq(int, EqFilter::FilterType, double, double, double)));

	balanceController = new BalanceController(this);
	connect(balanceController, SIGNAL(parameterChanged(size_t, float)), this, SLOT(onChangeBalance(size_t, float)));

	connect(ui->volumeSlider, SIGNAL(valueChanged(int)), this, SLOT(onChangeVolume(int)));
	connect(ui->delaySpinBox, SIGNAL(valueChanged(double)), this, SLOT(onChangeDelay(double)));
	connect(ui->sampleRateSpinBox, SIGNAL(valueChanged(int)), this, SLOT(onChangeClockDrift()));
	connect(ui->clockDriftSpinBox, SIGNAL(valueChanged(double)), this, SLOT(onChangeClockDrift()));
	connect(ui->enableCheckBox, SIGNAL(stateChanged(int)), this, SLOT(onEnable(int)));
	connect(ui->muteButton, SIGNAL(toggled(bool)), this, SLOT(onMute(bool)));

	connect(ui->eqButton, SIGNAL(clicked(bool)), this, SLOT(onShowEq()));
	connect(ui->compressorButton, SIGNAL(clicked(bool)), this, SLOT(onShowCompressor()));
	connect(ui->balanceButton, SIGNAL(clicked(bool)), this, SLOT(onShowBalance()));

	connect(&interface, SIGNAL(onMessage(QJsonObject)), this, SLOT(onMessageReceived(const QJsonObject&)));

	compressorController = new CompressorController(this);
	connect(compressorController,
	        SIGNAL(parameterChanged(bool, float, float, float, float, float, float)),
	        this,
	        SLOT(onChangeCompressor(bool, float, float, float, float, float, float)));

	ui->levelLabel->setTextSize(4, Qt::AlignLeft | Qt::AlignVCenter);
	ui->volumeLevelLabel->setTextSize(4, Qt::AlignRight | Qt::AlignVCenter);
}

OutputController::~OutputController() {
	delete ui;
}

void OutputController::setInterface(int index, WavePlayInterface* interface) {
	this->interface.setInterface(index, interface);
}

void OutputController::onChangeVolume(int volume) {
	qDebug("Changing volume");
	QJsonObject json;
	json["volume"] = volume;
	interface.sendMessage(json);
	setDisplayedVolume(volume);
}

void OutputController::onChangeDelay(double delay) {
	QJsonObject json;
	QJsonObject jsonDelay;
	jsonDelay["delay"] = delay;
	json["delayFilter"] = jsonDelay;
	interface.sendMessage(json);
}

void OutputController::onChangeClockDrift() {
	QJsonObject json;
	json["clockDrift"] = ui->clockDriftSpinBox->value() / 1000000 + ui->sampleRateSpinBox->value();
	interface.sendMessage(json);
}

void OutputController::onEnable(int state) {
	qDebug("Enabling / Disabling");
	QJsonObject json;
	if(state == Qt::Unchecked) {
		json["enabled"] = false;
		ui->groupBox->setDisabled(true);

		for(LevelMeterWidget* level : levelWidgets) {
			level->setValue(translateLevel(-INFINITY));
		}
		ui->levelLabel->setText("--");
	} else {
		json["enabled"] = true;
		ui->groupBox->setDisabled(false);
	}
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

void OutputController::onShowCompressor() {
	if(compressorController->isHidden()) {
		compressorController->show();
	} else {
		compressorController->hide();
	}
}

void OutputController::onShowBalance() {
	if(balanceController->isHidden()) {
		balanceController->show();
	} else {
		balanceController->hide();
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

void OutputController::onChangeCompressor(bool enabled,
                                          float releaseTime,
                                          float attackTime,
                                          float threshold,
                                          float makeUpGain,
                                          float compressionRatio,
                                          float kneeWidth) {
	qDebug("Changing compressor");

	QJsonObject json;
	QJsonObject filter;

	filter["enabled"] = enabled;
	filter["releaseTime"] = releaseTime;
	filter["attackTime"] = attackTime;
	filter["threshold"] = threshold;
	filter["makeUpGain"] = makeUpGain;
	filter["compressionRatio"] = compressionRatio;
	filter["kneeWidth"] = kneeWidth;

	json["compressorFilter"] = filter;

	interface.sendMessage(json);
}

void OutputController::onChangeBalance(size_t channel, float balance) {
	qDebug("Changing balance %d", (int) channel);

	QJsonObject json;
	QJsonArray balanceArray;
	QJsonObject balanceObject;

	balanceObject["channel"] = (int) channel;
	balanceObject["volume"] = balance;

	balanceArray.append(balanceObject);
	json["balance"] = balanceArray;

	interface.sendMessage(json);
}

void OutputController::onMessageReceived(const QJsonObject& message) {
	QJsonValue levelValue = message.value("levels");
	if(levelValue.type() == QJsonValue::Array) {
		if(ui->groupBox->isEnabled()) {
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
		}
	} else {
		qDebug("Received message %s", QJsonDocument(message).toJson(QJsonDocument::Indented).constData());

		QJsonValue numChannelsValue = message.value("numChannels");
		if(numChannelsValue.type() != QJsonValue::Undefined) {
			setNumChannel(numChannelsValue.toInt());
		}

		QJsonValue enableValue = message.value("enabled");
		if(enableValue.type() != QJsonValue::Undefined) {
			ui->enableCheckBox->blockSignals(true);
			ui->enableCheckBox->setChecked(enableValue.toBool());
			ui->enableCheckBox->blockSignals(false);
			ui->groupBox->setDisabled(!enableValue.toBool());
		}

		QJsonValue muteValue = message.value("mute");
		if(muteValue.type() != QJsonValue::Undefined) {
			ui->muteButton->blockSignals(true);
			ui->muteButton->setChecked(muteValue.toBool());
			ui->muteButton->blockSignals(false);
			for(size_t i = 0; i < levelWidgets.size(); i++) {
				levelWidgets[i]->setDisabled(muteValue.toBool());
			}
		}

		QJsonValue volumeValue = message.value("volume");
		if(volumeValue.type() != QJsonValue::Undefined) {
			int integerVolume = round(volumeValue.toDouble());
			ui->volumeSlider->blockSignals(true);
			ui->volumeSlider->setValue(integerVolume);
			ui->volumeSlider->blockSignals(false);
			setDisplayedVolume(integerVolume);
		}

		QJsonValue balanceValue = message.value("balance");
		if(balanceValue.type() == QJsonValue::Array) {
			QJsonArray balanceArray = balanceValue.toArray();
			for(int i = 0; i < balanceArray.size(); i++) {
				QJsonObject balanceInfo = balanceArray[i].toObject();

				balanceController->setParameters(balanceInfo["channel"].toInt(), balanceInfo["volume"].toDouble());
			}
		}

		QJsonValue delayValue = message.value("delayFilter");
		if(delayValue.type() != QJsonValue::Undefined) {
			ui->delaySpinBox->blockSignals(true);
			ui->delaySpinBox->setValue(delayValue.toObject()["delay"].toDouble());
			ui->delaySpinBox->blockSignals(false);
		}

		QJsonValue clockDriftValue = message.value("clockDrift");
		if(clockDriftValue.type() != QJsonValue::Undefined) {
			double drift = clockDriftValue.toDouble();
			ui->sampleRateSpinBox->blockSignals(true);
			ui->clockDriftSpinBox->blockSignals(true);
			ui->sampleRateSpinBox->setValue(roundf(drift));
			ui->clockDriftSpinBox->setValue((drift - roundf(drift)) * 1000000.0);
			ui->clockDriftSpinBox->blockSignals(false);
			ui->sampleRateSpinBox->blockSignals(false);
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

		QJsonValue compressorValue = message.value("compressorFilter");
		if(compressorValue.type() == QJsonValue::Object) {
			QJsonObject filterData = compressorValue.toObject();
			compressorController->blockSignals(true);
			compressorController->setParameters(filterData["enabled"].toBool(),
			                                    filterData["releaseTime"].toDouble(),
			                                    filterData["attackTime"].toDouble(),
			                                    filterData["threshold"].toDouble(),
			                                    filterData["makeUpGain"].toDouble(),
			                                    filterData["compressionRatio"].toDouble(),
			                                    filterData["kneeWidth"].toDouble());
			compressorController->blockSignals(false);
		}

		QJsonValue nameValue = message.value("name");
		if(nameValue.type() == QJsonValue::String && ui->groupBox->title().isEmpty()) {
			QString title = nameValue.toString().replace("waveSendUDP-", "");
			ui->groupBox->setTitle(title);
			equalizersController->setWindowTitle(tr("Equalizer - %1").arg(title));
			compressorController->setWindowTitle(tr("Compressor - %1").arg(title));
			balanceController->setWindowTitle(tr("Balance - %1").arg(title));
		}

		if(ui->groupBox->toolTip().isEmpty()) {
			QJsonValue deviceValue = message.value("device");
			QJsonValue ipValue = message.value("ip");
			QJsonValue portValue = message.value("port");

			if(deviceValue.type() == QJsonValue::String)
				ui->groupBox->setToolTip("Device: " + deviceValue.toString());
			else if(ipValue.type() == QJsonValue::String)
				ui->groupBox->setToolTip("Endpoint: " + ipValue.toString() + ":" + QString::number(portValue.toInt()));
		}
	}
}

double OutputController::translateLevel(double level) {
	if(level < -80)
		return -80;
	return level;
}

void OutputController::setDisplayedVolume(int volume) {
	ui->volumeLevelLabel->setText(QString::number(volume));
}

void OutputController::setNumChannel(int numChannels) {
	if(numChannels == this->numChannels) {
		return;
	}

	this->numChannels = numChannels;

	balanceController->setChannelNumber(numChannels);

	for(auto* w : levelWidgets) {
		ui->levelContainerLayout->removeWidget(w);
		delete w;
	}
	levelWidgets.clear();

	for(int i = 0; i < numChannels; i++) {
		//		QwtThermo* level = new QwtThermo(this);
		//		level->setObjectName(QStringLiteral("level") + QString::number(i + 1));
		//		level->setLowerBound(-80);
		//		level->setUpperBound(6);
		//		level->setScalePosition(/*i == 0 ? QwtThermo::TrailingScale :*/ QwtThermo::NoScale);
		//		level->setAlarmEnabled(true);
		//		level->setSpacing(0);
		//		level->setBorderWidth(0);
		//		level->setPipeWidth(3);
		//		level->setScaleMaxMinor(10);
		//		level->setScaleMaxMajor(0);
		//		level->setValue(translateLevel(-INFINITY));
		//		level->setOriginMode(QwtThermo::OriginMinimum);

		LevelMeterWidget* level = new LevelMeterWidget(this);
		levelWidgets.push_back(level);
		ui->levelContainerLayout->addWidget(level);
	}
}

void OutputController::dragEnterEvent(QDragEnterEvent* event) {
	if(event->mimeData()->hasFormat(DRAG_FORMAT)) {
		if(event->source() == this) {
			event->setDropAction(Qt::MoveAction);
			event->accept();
			qDebug("Drag enter self");
		} else {
			event->acceptProposedAction();
			qDebug("Drag enter other: %d", interface.getIndex());
		}
	} else {
		event->ignore();
	}
}

void OutputController::dragMoveEvent(QDragMoveEvent* event) {
	if(event->mimeData()->hasFormat(DRAG_FORMAT)) {
		if(event->source() == this) {
			event->setDropAction(Qt::MoveAction);
			event->accept();
			// qDebug("Drag move self");
		} else {
			event->acceptProposedAction();
			// qDebug("Drag move other: %d", interface.getIndex());

			QByteArray itemData = event->mimeData()->data(DRAG_FORMAT);
			QDataStream dataStream(&itemData, QIODevice::ReadOnly);
			int sourceInstance = -1;
			dataStream >> sourceInstance;

			mainWindow->moveOutputInstance(sourceInstance, interface.getIndex(), event->pos().x() < width() / 2);
		}
	} else {
		event->ignore();
	}
}

void OutputController::dropEvent(QDropEvent* event) {
	if(event->mimeData()->hasFormat(DRAG_FORMAT)) {
		QByteArray itemData = event->mimeData()->data(DRAG_FORMAT);
		QDataStream dataStream(&itemData, QIODevice::ReadOnly);

		if(event->source() == this) {
			event->setDropAction(Qt::MoveAction);
			event->accept();
			qDebug("Drag drop self");
		} else {
			event->acceptProposedAction();
			qDebug("Drag drop other: %d", interface.getIndex());
		}
	} else {
		event->ignore();
	}
}

void OutputController::mousePressEvent(QMouseEvent* event) {
	QByteArray itemData;
	QDataStream dataStream(&itemData, QIODevice::WriteOnly);
	dataStream << interface.getIndex();

	QMimeData* mimeData = new QMimeData;
	mimeData->setData(DRAG_FORMAT, itemData);

	QDrag* drag = new QDrag(this);
	drag->setMimeData(mimeData);

	qDebug("Begin drag %d", interface.getIndex());
	if(drag->exec(Qt::MoveAction) == Qt::MoveAction) {
		qDebug("End drag");
	} else {
		qDebug("End no drag");
	}
}
