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

OutputController::OutputController(MainWindow* parent, OscContainer* oscParent, const std::string& name)
    : QWidget(parent),
      OscContainer(oscParent, name),
      ui(new Ui::OutputController),
      mainWindow(parent),
      oscFilterChain(this, "filterChain"),
      oscMeterPerChannel(this, "meter_per_channel"),
      oscEnable(this, "enable"),
      oscMute(&oscFilterChain, "mute"),
      oscDelay(&oscFilterChain, "delay"),
      oscClockDrift(&oscFilterChain, "clockDrift"),
      oscVolume(&oscFilterChain, "volume"),
      oscDisplayName(this, "display_name"),
      sampleRate(this, "sample_rate") {
	ui->setupUi(this);

	numChannels = 0;

	oscEnable.setWidget(ui->enableCheckBox);
	oscMute.setWidget(ui->muteButton);
	oscDelay.setWidget(ui->delaySpinBox);
	oscClockDrift.setWidget(ui->clockDriftSpinBox);
	oscVolume.setWidget(ui->volumeSlider);
	oscVolume.setChangeCallback([this](float value) { ui->volumeLevelLabel->setText(QString::number((int) value)); });

	equalizersController = new EqualizersController(this, &oscFilterChain, 6);

	balanceController = new BalanceController(this, &oscFilterChain);

	connect(parent, &MainWindow::showDisabledChanged, this, &OutputController::updateHiddenState);
	connect(ui->enableCheckBox, &QCheckBox::toggled, this, &OutputController::updateHiddenState);
	oscEnable.setChangeCallback([this](float) { updateHiddenState(); });

	connect(ui->eqButton, &QAbstractButton::clicked, this, &OutputController::onShowEq);
	connect(ui->compressorButton, &QAbstractButton::clicked, this, &OutputController::onShowCompressor);
	connect(ui->expanderButton, &QAbstractButton::clicked, this, &OutputController::onShowExpander);
	connect(ui->balanceButton, &QAbstractButton::clicked, this, &OutputController::onShowBalance);

	compressorController = new CompressorController(this, &oscFilterChain, "compressorFilter");
	compressorController->getEnableMapper().setWidget(ui->compressorButton, false);

	expanderController = new CompressorController(this, &oscFilterChain, "expanderFilter");
	expanderController->getEnableMapper().setWidget(ui->expanderButton, false);

	ui->levelLabel->setTextSize(4, Qt::AlignLeft | Qt::AlignVCenter);
	ui->volumeLevelLabel->setTextSize(4, Qt::AlignRight | Qt::AlignVCenter);

	oscDisplayName.setChangeCallback([this](const std::string& value) {
		QString title = QString::fromStdString(value).replace("waveSendUDP-", "");
		ui->groupBox->setTitle(title);
		equalizersController->setWindowTitle(tr("Equalizer - %1").arg(title));
		compressorController->setWindowTitle(tr("Compressor - %1").arg(title));
		expanderController->setWindowTitle(tr("Expander - %1").arg(title));
		balanceController->setWindowTitle(tr("Balance - %1").arg(title));
	});

	oscMeterPerChannel.setCallback([this](const std::vector<OscArgument>& arguments) {
		float maxLevel = -INFINITY;

		setNumChannel(arguments.size());

		size_t levelNumber = std::min(levelWidgets.size(), arguments.size());

		for(size_t i = 0; i < levelNumber; i++) {
			const auto& argument = arguments[i];
			float level;

			if(!OscNode::getArgumentAs<float>(argument, level))
				level = 0;

			levelWidgets[i]->setValue(level);
			if(level > maxLevel)
				maxLevel = level;
		}

		if(maxLevel <= -192)
			ui->levelLabel->setText("--");
		else
			ui->levelLabel->setText(QString::number((int) maxLevel));
	});
}

OutputController::~OutputController() {
	delete ui;
}

void OutputController::setInterface(WavePlayInterface* interface) {
	this->interface.setInterface(0, interface);
}

void OutputController::updateHiddenState() {
	bool hide = !mainWindow->getShowDisabledOutputInstances() && ui->enableCheckBox->isChecked() == false;
	setHidden(hide);
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

void OutputController::onMute(bool muted) {
	qDebug("Muting");
	QJsonObject json;
	json["mute"] = muted;
	interface.sendMessage(json);
}

void OutputController::onShowEq() {
	ui->eqButton->toggle();
	if(equalizersController->isHidden()) {
		equalizersController->show();
	} else {
		equalizersController->hide();
	}
}

void OutputController::onShowCompressor() {
	ui->compressorButton->toggle();
	if(compressorController->isHidden()) {
		compressorController->show();
	} else {
		compressorController->hide();
	}
}

void OutputController::onShowExpander() {
	ui->expanderButton->toggle();
	if(expanderController->isHidden()) {
		expanderController->show();
	} else {
		expanderController->hide();
	}
}

void OutputController::onShowBalance() {
	if(balanceController->isHidden()) {
		balanceController->show();
	} else {
		balanceController->hide();
	}
}

void OutputController::onChangeEq(int index, bool enabled, FilterType type, double f0, double q, double gain) {
	qDebug("Changing eq %d", index);

	QJsonObject json;
	QJsonArray eqFiltersArray;
	QJsonObject eqFilter;

	eqFilter["index"] = index;
	eqFilter["enabled"] = enabled;
	eqFilter["type"] = (int) type;
	eqFilter["f0"] = f0;
	eqFilter["Q"] = q;
	eqFilter["gain"] = gain;

	eqFiltersArray.append(eqFilter);
	json["eqFilters"] = eqFiltersArray;

	interface.sendMessage(json);
}

void OutputController::sendChangeCompressor(const char* filterName,
                                            bool enabled,
                                            float releaseTime,
                                            float attackTime,
                                            float threshold,
                                            float makeUpGain,
                                            float ratio,
                                            float kneeWidth,
                                            bool useMovingMax) {
	qDebug("Changing compressor");

	QJsonObject json;
	QJsonObject filter;

	filter["enabled"] = enabled;
	filter["releaseTime"] = releaseTime;
	filter["attackTime"] = attackTime;
	filter["threshold"] = threshold;
	filter["makeUpGain"] = makeUpGain;
	filter["ratio"] = ratio;
	filter["kneeWidth"] = kneeWidth;
	filter["useMovingMax"] = useMovingMax;

	json[filterName] = filter;

	interface.sendMessage(json);
}

void OutputController::onChangeCompressor(bool enabled,
                                          float releaseTime,
                                          float attackTime,
                                          float threshold,
                                          float makeUpGain,
                                          float ratio,
                                          float kneeWidth,
                                          bool useMovingMax) {
	sendChangeCompressor(
	    "compressorFilter", enabled, releaseTime, attackTime, threshold, makeUpGain, ratio, kneeWidth, useMovingMax);
}

void OutputController::onChangeExpander(bool enabled,
                                        float releaseTime,
                                        float attackTime,
                                        float threshold,
                                        float makeUpGain,
                                        float ratio,
                                        float kneeWidth,
                                        bool useMovingMax) {
	sendChangeCompressor(
	    "expanderFilter", enabled, releaseTime, attackTime, threshold, makeUpGain, ratio, kneeWidth, useMovingMax);
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
	return;
	if(levelValue.type() == QJsonValue::Array) {
		if(ui->groupBox->isEnabled()) {
			QJsonArray levels = levelValue.toArray();
			double maxLevel = -INFINITY;
			int levelNumber = std::min(levels.size(), (int) levelWidgets.size());

			for(int i = 0; i < levelNumber; i++) {
				double level = levels[i].toDouble();
				levelWidgets[i]->setValue(level);
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
			updateHiddenState();
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

		QJsonValue displayNameValue = message.value("displayName");
		if(displayNameValue.type() == QJsonValue::String) {
		}

		if(ui->groupBox->toolTip().isEmpty()) {
			QJsonValue nameValue = message.value("name");
			QJsonValue deviceValue = message.value("device");
			QJsonValue ipValue = message.value("ip");
			QJsonValue portValue = message.value("port");

			QString title = nameValue.toString("").replace("waveSendUDP-", "");

			if(deviceValue.type() == QJsonValue::String)
				ui->groupBox->setToolTip(title + " Device: " + deviceValue.toString());
			else if(ipValue.type() == QJsonValue::String)
				ui->groupBox->setToolTip(title + " Endpoint: " + ipValue.toString() + ":" +
				                         QString::number(portValue.toInt()));
			else
				ui->groupBox->setToolTip(title);
		}
	}
}

void OutputController::setDisplayedVolume(int volume) {}

void OutputController::setNumChannel(int numChannels) {
	if(numChannels == this->numChannels) {
		return;
	}

	this->numChannels = numChannels;

	for(auto* w : levelWidgets) {
		ui->levelContainerLayout->removeWidget(w);
		delete w;
	}
	levelWidgets.clear();

	for(int i = 0; i < numChannels; i++) {
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
			QString sourceInstance;
			dataStream >> sourceInstance;

			mainWindow->moveOutputInstance(sourceInstance.toStdString(), getName(), event->pos().x() < width() / 2);
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
	dataStream << QString::fromStdString(getName());

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
