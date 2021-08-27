#include "MainWindow.h"
#include "ui_MainWindow.h"
#include <QInputDialog>
#include <QJsonDocument>

MainWindow::MainWindow(QWidget* parent)
    : QWidget(parent),
      ui(new Ui::MainWindow),
      oscRoot(false),
      wavePlayInterface(&oscRoot),
      outputInterfaces(&oscRoot, "strip") {
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
	clearOutputs();
	delete ui;
}

void MainWindow::onMessage(const QJsonObject& message) {
	qDebug("Received global message: %s", QJsonDocument(message).toJson().constData());
	QJsonValue outputInstance = message["instance"];
	if(outputInstance.type() != QJsonValue::Undefined && outputInstance.toInt() == -1) {
		if(message["operation"] == "outputList") {
			int numOutputInstances = message["numOutputInstances"].toInt();
			numEq = message["numEq"].toInt();

			if(numOutputInstances > 100) {
				printf("too many instance %d\n", numOutputInstances);
				return;
			}

			clearOutputs();

			printf("%d instances\n", numOutputInstances);
		} else if(message["operation"] == "queryResult" && openAddDialogRequested) {
			openAddDialogRequested = false;

			addDialog.setTypeList(message["typeList"].toArray());
			addDialog.setDeviceList(message["deviceList"].toArray());
#ifdef _WIN32
			addDialog.setWasapiDeviceList(message["deviceListWasapi"].toArray());
#endif
			addDialog.show();
		} else if(message["operation"] == "add") {
		} else if(message["operation"] == "remove") {
		}
	}
}

void MainWindow::onAddInstance() {
	openAddDialogRequested = true;

	QJsonObject json;
	json["operation"] = "query";
}

void MainWindow::onRemoveInstance() {
	bool ok;

	int value =
	    QInputDialog::getInt(this, "Remove item index", "Put the device index to remove", 0, 0, 2147483647, 1, &ok);

	if(ok) {
		QJsonObject json;
		json["operation"] = "remove";
		json["target"] = value;
	}
}

void MainWindow::clearOutputs() {
	for(auto& output : outputs) {
		ui->horizontalLayout->removeWidget(output.second);
		delete output.second;
	}
	outputs.clear();
}

void MainWindow::moveOutputInstance(int sourceInstance, int targetInstance, bool insertBefore) {
	outputInterfaces.swapWidgets(sourceInstance, targetInstance, insertBefore, true);
}

bool MainWindow::getShowDisabledOutputInstances() {
	return ui->showDisabledCheckBox->isChecked();
}
