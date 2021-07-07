#include "MainWindow.h"
#include "ui_MainWindow.h"
#include <QInputDialog>
#include <QJsonDocument>

MainWindow::MainWindow(QWidget* parent) : QWidget(parent), ui(new Ui::MainWindow), addDialog(&mainControlInterface) {
	ui->setupUi(this);

	mainControlInterface.setInterface(-1, &wavePlayInterface);

	connect(&mainControlInterface, SIGNAL(onMessage(QJsonObject)), this, SLOT(onMessage(const QJsonObject&)));
	connect(ui->addButton, SIGNAL(clicked(bool)), this, SLOT(onAddInstance()));
	connect(ui->removeButton, SIGNAL(clicked(bool)), this, SLOT(onRemoveInstance()));
	connect(ui->showDisabledCheckBox, SIGNAL(toggled(bool)), this, SLOT(onShowDisabled(bool)));
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
			int index = message["target"].toInt(-1);

			qDebug("Adding instance %d\n", index);

			if(index >= 0) {
				OutputController* output = new OutputController(this, numEq);
				output->setInterface(index, &wavePlayInterface);
				ui->horizontalLayout->addWidget(output);

				outputs.emplace(index, output);
			}
		} else if(message["operation"] == "remove") {
			int index = message["target"].toInt(-1);

			if(index >= 0) {
				auto it = outputs.find(index);
				if(it != outputs.end()) {
					ui->horizontalLayout->removeWidget(it->second);
					delete it->second;
					outputs.erase(it);
				}
			}
		}
	}
}

void MainWindow::onAddInstance() {
	openAddDialogRequested = true;

	QJsonObject json;
	json["operation"] = "query";
	mainControlInterface.sendMessage(json);
}

void MainWindow::onRemoveInstance() {
	bool ok;

	int value =
	    QInputDialog::getInt(this, "Remove item index", "Put the device index to remove", 0, 0, 2147483647, 1, &ok);

	if(ok) {
		QJsonObject json;
		json["operation"] = "remove";
		json["target"] = value;
		mainControlInterface.sendMessage(json);
	}
}

void MainWindow::onShowDisabled(bool showDisabled) {
	showDisabledOutputInstances = showDisabled;
	for(auto& output : outputs) {
		output.second->updateHiddenState();
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
	auto itSource = outputs.find(sourceInstance);
	auto itTarget = outputs.find(targetInstance);
	if(itSource == outputs.end() || itTarget == outputs.end())
		return;
	OutputController* sourceWidget = itSource->second;
	int originalPosition = ui->horizontalLayout->indexOf(sourceWidget);
	ui->horizontalLayout->removeWidget(sourceWidget);

	int insertPosition = ui->horizontalLayout->indexOf(itTarget->second);
	if(insertPosition >= 0) {
		if(!insertBefore)
			insertPosition++;
		qDebug("Move instance %d from pos %d to %d (over instance %d)",
		       sourceInstance,
		       originalPosition,
		       insertPosition,
		       targetInstance);
		ui->horizontalLayout->insertWidget(insertPosition, sourceWidget);

		QJsonObject json;
		json["operation"] = "outputsOrder";

		QJsonArray array;
		for(int i = 0; i < ui->horizontalLayout->count(); i++) {
			OutputController* outputInstance = (OutputController*) ui->horizontalLayout->itemAt(i)->widget();
			array.append(outputInstance->getOutputInterface()->getIndex());
		}
		json["outputsOrder"] = array;

		mainControlInterface.sendMessage(json);
	} else {
		qDebug("Move instance %d from pos %d to %d (original pos)", sourceInstance, originalPosition, originalPosition);
		ui->horizontalLayout->insertWidget(originalPosition, sourceWidget);
	}
}
