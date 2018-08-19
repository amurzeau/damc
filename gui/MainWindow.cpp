#include "MainWindow.h"
#include "ui_MainWindow.h"

MainWindow::MainWindow(QWidget* parent) : QWidget(parent), ui(new Ui::MainWindow) {
	ui->setupUi(this);

	mainControlInterface.setInterface(-1, &wavePlayInterface);

	connect(&mainControlInterface,
	        SIGNAL(onMessage(notification_message_t)),
	        this,
	        SLOT(onMessage(notification_message_t)));
}

MainWindow::~MainWindow() {
	delete ui;
	for(OutputController* output : outputs) {
		layout()->removeWidget(output);
		delete output;
	}
	outputs.clear();
}

void MainWindow::onMessage(notification_message_t message) {
	if(message.opcode == notification_message_t::op_state) {
		if(message.data.state.numOutputInstances > 20) {
			printf("too many instance %d\n", message.data.state.numOutputInstances);
			return;
		}

		for(OutputController* output : outputs) {
			layout()->removeWidget(output);
			delete output;
		}
		outputs.clear();

		printf("%d instances\n", message.data.state.numOutputInstances);
		for(int i = 0; i < message.data.state.numOutputInstances; i++) {
			OutputController* output =
			    new OutputController(this, message.data.state.numEq, message.data.state.numChannels);
			output->setInterface(i, &wavePlayInterface);
			ui->horizontalLayout->addWidget(output);

			outputs.push_back(output);
		}
	}
}
