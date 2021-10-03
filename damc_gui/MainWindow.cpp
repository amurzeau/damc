#include "MainWindow.h"
#include "GlobalConfigDialog.h"
#include "OutputController.h"
#include "ui_MainWindow.h"
#include <QInputDialog>

MainWindow::MainWindow(QWidget* parent)
    : QWidget(parent),
      ui(new Ui::MainWindow),
      oscRoot(false),
      wavePlayInterface(&oscRoot),
      outputInterfaces(&oscRoot, "strip"),
      oscTypeArray(&oscRoot, "type_list"),
      oscPortaudioDeviceArray(&oscRoot, "device_list"),
      oscWasapiDeviceArray(&oscRoot, "device_list_wasapi") {
	ui->setupUi(this);

	globalConfigDialog = new GlobalConfigDialog(this, &oscRoot);
	connect(globalConfigDialog, &GlobalConfigDialog::showStateChanged, [this](bool shown) {
		ui->globalConfigButton->setChecked(shown);
	});

	outputInterfaces.setWidget(
	    this, ui->horizontalLayout, [this](QWidget*, OscContainer* oscParent, int name) -> QWidget* {
		    return new OutputController(this, oscParent, std::to_string(name));
	    });

	connect(ui->globalConfigButton, &QAbstractButton::clicked, this, &MainWindow::onShowGlobalConfig);
	connect(ui->addButton, &QAbstractButton::clicked, this, &MainWindow::onAddInstance);
	connect(ui->removeButton, &QAbstractButton::clicked, this, &MainWindow::onRemoveInstance);
	connect(ui->showDisabledCheckBox, &QCheckBox::toggled, this, &MainWindow::showDisabledChanged);

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
	oscRoot.sendMessage("/device_list/dump", nullptr, 0);
	oscRoot.sendMessage("/device_list_wasapi/dump", nullptr, 0);
}
