#include "MainWindow.h"
#include "ui_MainWindow.h"
#include <QJsonDocument>

MainWindow::MainWindow(QWidget* parent) : QWidget(parent), ui(new Ui::MainWindow) {
	ui->setupUi(this);

	mainControlInterface.setInterface(-1, &wavePlayInterface);

	connect(&mainControlInterface, SIGNAL(onMessage(QJsonObject)), this, SLOT(onMessage(const QJsonObject&)));
}

MainWindow::~MainWindow() {
	delete ui;
	for(OutputController* output : outputs) {
		layout()->removeWidget(output);
		delete output;
	}
	outputs.clear();
}

void MainWindow::onMessage(const QJsonObject& message) {
	qDebug("Received global message: %s", QJsonDocument(message).toJson().constData());
	QJsonValue outputInstance = message["instance"];
	if(outputInstance.type() != QJsonValue::Undefined && outputInstance.toInt() == -1) {
		int numOutputInstances = message["numOutputInstances"].toInt();
		int numEq = message["numEq"].toInt();
		int numChannels = message["numChannels"].toInt();

		if(numOutputInstances > 20) {
			printf("too many instance %d\n", numOutputInstances);
			return;
		}

		for(OutputController* output : outputs) {
			layout()->removeWidget(output);
			delete output;
		}
		outputs.clear();

		printf("%d instances\n", numOutputInstances);
		for(int i = 0; i < numOutputInstances; i++) {
			OutputController* output = new OutputController(this, numEq, numChannels);
			output->setInterface(i, &wavePlayInterface);
			ui->horizontalLayout->addWidget(output);

			outputs.push_back(output);
		}
	}
}
