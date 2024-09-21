#include "MainWindow.h"
#include "GlobalConfigDialog.h"
#include "OutputController.h"
#include "qabstractbutton.h"
#include "qmessagebox.h"
#include "ui_MainWindow.h"
#include <QFileDialog>
#include <QInputDialog>

MainWindow::MainWindow(OscRoot* oscRoot, bool isMicrocontrollerDamc, QWidget* parent)
    : QWidget(parent),
      ui(new Ui::MainWindow),
      oscRoot(oscRoot),
      statePersist(oscRoot),
      isMicrocontrollerDamc(isMicrocontrollerDamc),
      outputInterfaces(oscRoot, "strip"),
      oscTypeArray(oscRoot, "type_list"),
      oscPortaudioDeviceArray(oscRoot, "device_list"),
      oscWasapiDeviceArray(oscRoot, "device_list_wasapi") {
	ui->setupUi(this);

	if(isMicrocontrollerDamc) {
		ui->showDisabledCheckBox->hide();
		ui->addButton->hide();
		ui->removeButton->hide();
	}

	globalConfigDialog = new GlobalConfigDialog(this, oscRoot);
	connect(globalConfigDialog, &GlobalConfigDialog::showStateChanged, [this](bool shown) {
		ui->globalConfigButton->setChecked(shown);
	});

	outputInterfaces.setWidget(
	    this, ui->horizontalLayout, [this](QWidget*, OscContainer* oscParent, int name) -> QWidget* {
		    return new OutputController(this, oscParent, std::to_string(name), this->isMicrocontrollerDamc);
	    });

	ui->globalConfigButton->setMenu(&configurationMenu);

	QAction* globalConfigurationAction = new QAction("&Global configuration...", this);
	connect(globalConfigurationAction, &QAction::triggered, this, &MainWindow::onShowGlobalConfig);
	configurationMenu.addAction(globalConfigurationAction);

	QAction* configurationSaveAction = new QAction("&Save configuration", this);
	connect(configurationSaveAction, &QAction::triggered, this, &MainWindow::onSaveConfiguration);
	configurationMenu.addAction(configurationSaveAction);

	QAction* configurationLoadAction = new QAction("&Load configuration", this);
	connect(configurationLoadAction, &QAction::triggered, this, &MainWindow::onLoadConfiguration);
	configurationMenu.addAction(configurationLoadAction);

	connect(ui->addButton, &QAbstractButton::clicked, this, &MainWindow::onAddInstance);
	connect(ui->removeButton, &QAbstractButton::clicked, this, &MainWindow::onRemoveInstance);
	connect(ui->showDisabledCheckBox, &QCheckBox::toggled, [this](bool showDisabled) {
		emit showDisabledChanged();
		if(!showDisabled) {
			// These 2 processEvents are required so the resize is really taken into account by Qt
			QApplication::processEvents();
			QApplication::processEvents();
			this->resize(minimumWidth(), this->height());
		}
	});

	connect(ui->audioGlitchToolButton, &QAbstractButton::clicked, this, &MainWindow::onGlitchButtonPressed);
	connect(globalConfigDialog, &GlobalConfigDialog::glitchOccurred, this, &MainWindow::onGlitchOccurred);

	oscTypeArray.addChangeCallback([this](const auto&, const auto&) { emit typeListChanged(); });

	oscPortaudioDeviceArray.addChangeCallback([this](const auto&, const auto&) { emit deviceListChanged(); });

	oscWasapiDeviceArray.addChangeCallback([this](const auto&, const auto&) { emit deviceListChanged(); });
}

MainWindow::~MainWindow() {
	delete ui;
}

void MainWindow::onShowGlobalConfig() {
	if(globalConfigDialog->isHidden()) {
		globalConfigDialog->show();
		ui->globalConfigButton->setChecked(true);
	} else {
		globalConfigDialog->hide();
		ui->globalConfigButton->setChecked(false);
	}
}

void MainWindow::onSaveConfiguration() {
	QString fileName =
	    QFileDialog::getSaveFileName(this, tr("Save Config file"), "", tr("DAMC Config (*.json);;All files (*)"));
	if(fileName.isEmpty())
		return;

	try {
		statePersist.saveState(fileName.toStdString());
	} catch(const std::exception& e) {
		QMessageBox::critical(this, "Failed to save config", QString("Error while parsing config: %1").arg(e.what()));
	} catch(...) {
		QMessageBox::critical(this, "Failed to save config", "Error while parsing config");
	}
}

void MainWindow::onLoadConfiguration() {
	QString fileName =
	    QFileDialog::getOpenFileName(this, tr("Open Config file"), "", tr("DAMC Config (*.json);;All files (*)"));
	if(fileName.isEmpty())
		return;

	try {
		statePersist.loadState(fileName.toStdString());
	} catch(const std::exception& e) {
		QMessageBox::critical(this, "Failed to load config", QString("Error while parsing config: %1").arg(e.what()));
	} catch(...) {
		QMessageBox::critical(this, "Failed to load config", "Error while parsing config");
	}
}

void MainWindow::onAddInstance() {
	int key = outputInterfaces.addItem();
	QWidget* outputInstance = outputInterfaces.getWidget(key);
	if(outputInstance) {
		OutputController* outputController = (OutputController*) outputInstance;
		outputController->showConfigDialog();
		outputController->show();
	}
}

void MainWindow::onRemoveInstance() {
	bool ok;

	int value =
	    QInputDialog::getInt(this, "Remove item index", "Put the device index to remove", 0, 0, 2147483647, 1, &ok);

	if(ok) {
		outputInterfaces.removeItem(value);
	}
}

void MainWindow::onGlitchOccurred() {
	ui->audioGlitchToolButton->setStyleSheet("color: red;");
}

void MainWindow::onGlitchButtonPressed() {
	// Reset button color
	ui->audioGlitchToolButton->setStyleSheet("");
	globalConfigDialog->resetGlitchCounters();
}

void MainWindow::removeInstance(int key) {
	outputInterfaces.removeItem(key);
}

void MainWindow::moveOutputInstance(int sourceInstance, int targetInstance, bool insertBefore) {
	outputInterfaces.swapWidgets(sourceInstance, targetInstance, insertBefore, true);
}

bool MainWindow::getShowDisabledOutputInstances() {
	return ui->showDisabledCheckBox->isChecked();
}

void MainWindow::updateDeviceList() {
	oscRoot->sendMessage("/device_list/dump", nullptr, 0);
	oscRoot->sendMessage("/device_list_wasapi/dump", nullptr, 0);
}
