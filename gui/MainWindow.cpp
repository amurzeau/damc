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

			if(numOutputInstances > 20) {
				printf("too many instance %d\n", numOutputInstances);
				return;
			}

			clearOutputs();

			printf("%d instances\n", numOutputInstances);
		} else if(message["operation"] == "queryResult" && openAddDialogRequested) {
			openAddDialogRequested = false;

			addDialog.setTypeList(message["typeList"].toArray());
			addDialog.setDeviceList(message["deviceList"].toArray());
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
					layout()->removeWidget(it->second);
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

void MainWindow::clearOutputs() {
	for(auto& output : outputs) {
		layout()->removeWidget(output.second);
		delete output.second;
	}
	outputs.clear();
}
