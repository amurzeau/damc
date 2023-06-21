#include "SerialPortInterface.h"
#include <QMessageBox>
#include <QSerialPortInfo>

SerialPortInterface::SerialPortInterface(OscRoot* oscRoot, QString portName)
    : OscConnector(oscRoot, true), oscSerialPort(portName) {
	connect(&oscSerialPort, &QIODevice::readyRead, this, &SerialPortInterface::onOscDataReceived);

	onOscReconnect();
}

void SerialPortInterface::updateOscVariables() {
	getOscRoot()->sendMessage("/**/dump", nullptr, 0);
}

void SerialPortInterface::onOscDataReceived() {
	do {
		// QByteArray receivedData = oscSerialPort.readAll();
		QByteArray receivedData;

		receivedData = oscSerialPort.readAll();
		printf("data: %s\n", receivedData.constData());
		OscConnector::onOscDataReceived((const uint8_t*) receivedData.data(), receivedData.size());
	} while(oscSerialPort.bytesAvailable());
}

// void SerialPortInterface::onOscConnectionStateChanged(QAbstractSocket::SocketState state) {
// 	if(state == QAbstractSocket::UnconnectedState) {
// 		oscReconnectTimer.singleShot(1000, this, &SerialPortInterface::onOscReconnect);
// 	} else if(state == QAbstractSocket::ConnectedState) {
// 		updateOscVariables();
// 	}
// }

void SerialPortInterface::onOscReconnect() {
#if 1
	QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();
	for(const auto& port : ports) {
		printf(
		    "available port: %s: %s\n", port.portName().toUtf8().constData(), port.description().toUtf8().constData());
	}

	if(!oscSerialPort.open(QIODevice::ReadWrite)) {
		printf("Open failed: %s\n", oscSerialPort.errorString().toUtf8().constData());
	}
	updateOscVariables();
#else
	oscSerialPort.bind(10000, QAbstractSocket::ReuseAddressHint);
#endif
}

void SerialPortInterface::sendOscData(const uint8_t* data, size_t size) {
	oscSerialPort.write((const char*) data, size);
}
