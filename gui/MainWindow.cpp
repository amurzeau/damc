#include "MainWindow.h"
#include "OutputController.h"
#include "ui_MainWindow.h"
#include <QInputDialog>

MainWindow::MainWindow(QWidget* parent)
    : QWidget(parent),
      ui(new Ui::MainWindow),
      oscRoot(false),
      wavePlayInterface(&oscRoot),
      outputInterfaces(&oscRoot, "strip"),
      addDialog(&oscRoot) {
	ui->setupUi(this);

	outputInterfaces.setWidget(
	    this, ui->horizontalLayout, [this](QWidget*, OscContainer* oscParent, int name) -> QWidget* {
		    return new OutputController(this, oscParent, std::to_string(name));
	    });

	connect(ui->addButton, &QAbstractButton::clicked, this, &MainWindow::onAddInstance);
	connect(ui->removeButton, &QAbstractButton::clicked, this, &MainWindow::onRemoveInstance);
	connect(ui->showDisabledCheckBox, &QCheckBox::toggled, this, &MainWindow::showDisabledChanged);
}

MainWindow::~MainWindow() {
	delete ui;
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
