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
#include <QMessageBox>
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
      oscVolume(&oscFilterChain, "volume"),
      oscName(this, "name"),
      oscDisplayName(this, "display_name"),
      oscSampleRate(this, "sample_rate"),
      oscJackSampleRate(this, "realSampleRate") {
	ui->setupUi(this);

	numChannels = 0;

	// By default hide, it will be shown if enabled
	hide();

	oscEnable.setWidget(ui->enableCheckBox);
	oscMute.setWidget(ui->muteButton);
	oscDelay.setWidget(ui->delaySpinBox);

	oscVolume.setWidget(ui->volumeSlider);
	oscVolume.setChangeCallback([this](int32_t value) { ui->volumeLevelLabel->setText(QString::number(value)); });

	equalizersController = new EqualizersController(this, &oscFilterChain);
	balanceController = new BalanceController(this, &oscFilterChain);

	compressorController = new CompressorController(this, &oscFilterChain, "compressorFilter");
	compressorController->getEnableMapper().setWidget(ui->compressorButton, false);

	expanderController = new CompressorController(this, &oscFilterChain, "expanderFilter");
	expanderController->getEnableMapper().setWidget(ui->expanderButton, false);

	configDialog = new OutputInstanceConfigDialog(parent, this);

	connect(parent, &MainWindow::showDisabledChanged, this, &OutputController::updateHiddenState);
	connect(ui->enableCheckBox, &QCheckBox::toggled, this, &OutputController::updateHiddenState);

	oscEnable.setChangeCallback([this](bool) {
		updateHiddenState();
		setNumChannel(0);
		ui->levelLabel->setText("--");
	});

	connect(ui->eqButton, &QAbstractButton::clicked, this, &OutputController::onShowEq);
	connect(ui->compressorButton, &QAbstractButton::clicked, this, &OutputController::onShowCompressor);
	connect(ui->expanderButton, &QAbstractButton::clicked, this, &OutputController::onShowExpander);
	connect(ui->balanceButton, &QAbstractButton::clicked, this, &OutputController::onShowBalance);
	connect(ui->configButton, &QAbstractButton::clicked, this, &OutputController::onShowConfig);
	connect(ui->removeButton, &QAbstractButton::clicked, this, &OutputController::onRemove);

	ui->levelLabel->setTextSize(4, Qt::AlignLeft | Qt::AlignVCenter);
	ui->volumeLevelLabel->setTextSize(4, Qt::AlignRight | Qt::AlignVCenter);

	oscMute.setChangeCallback([this](bool newValue) {
		for(size_t i = 0; i < levelWidgets.size(); i++) {
			levelWidgets[i]->setDisabled(newValue);
		}
	});

	oscName.setChangeCallback([this](const std::string&) { updateTooltip(); });

	oscDisplayName.setChangeCallback([this](const std::string& value) {
		QString title = QString::fromStdString(value);
		ui->groupBox->setTitle(title);
		equalizersController->setWindowTitle(tr("Equalizer - %1").arg(title));
		compressorController->setWindowTitle(tr("Compressor - %1").arg(title));
		expanderController->setWindowTitle(tr("Expander - %1").arg(title));
		balanceController->setWindowTitle(tr("Balance - %1").arg(title));
		configDialog->setWindowTitle(tr("Config - %1").arg(title));
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

void OutputController::showConfigDialog() {
	configDialog->show();
}

void OutputController::updateHiddenState() {
	// Hide when:
	// - Show disabled is not checked
	// - This output is not enabled
	// - And the config dialog is not opened
	bool hide = !mainWindow->getShowDisabledOutputInstances() && ui->enableCheckBox->isChecked() == false &&
	            configDialog->isHidden();
	setHidden(hide);
}

void OutputController::updateTooltip() {
	QString title = QString::fromStdString(oscName.get());

	ui->groupBox->setToolTip(title);
}

void OutputController::updateEqEnable() {
	ui->eqButton->setChecked(equalizersController->getEqEnabled());
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

void OutputController::onShowConfig() {
	if(configDialog->isHidden()) {
		configDialog->show();
	} else {
		configDialog->hide();
	}
}

void OutputController::onRemove() {
	QMessageBox msgBox;
	msgBox.setText(QString("Really remove %1 (%2)?")
	                   .arg(QString::fromStdString(oscDisplayName.get()), QString::fromStdString(oscName.get())));
	msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
	msgBox.setDefaultButton(QMessageBox::Cancel);
	int ret = msgBox.exec();

	switch(ret) {
		case QMessageBox::Ok:
			mainWindow->removeInstance(atoi(getName().c_str()));
			break;
		case QMessageBox::Cancel:
			// Cancel was clicked
			break;
		default:
			// should never be reached
			break;
	}
}

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
			qDebug("Drag drop other: %s", getName().c_str());
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
			int sourceInstance;
			dataStream >> sourceInstance;

			mainWindow->moveOutputInstance(sourceInstance, atoi(getName().c_str()), event->pos().x() < width() / 2);
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
			qDebug("Drag drop other: %s", getName().c_str());
		}
	} else {
		event->ignore();
	}
}

void OutputController::mousePressEvent(QMouseEvent* event) {
	QByteArray itemData;
	QDataStream dataStream(&itemData, QIODevice::WriteOnly);
	dataStream << atoi(getName().c_str());

	QMimeData* mimeData = new QMimeData;
	mimeData->setData(DRAG_FORMAT, itemData);

	QDrag* drag = new QDrag(this);
	drag->setMimeData(mimeData);

	qDebug("Begin drag: %s", getName().c_str());
	if(drag->exec(Qt::MoveAction) == Qt::MoveAction) {
		qDebug("End drag");
	} else {
		qDebug("End no drag");
	}
}
