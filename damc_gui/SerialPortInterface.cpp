#include "SerialPortInterface.h"
#include <QMessageBox>
#include <QSerialPortInfo>
#include <spdlog/fmt/bin_to_hex.h>
#include <spdlog/spdlog.h>

SerialPortInterface::SerialPortInterface(OscRoot* oscRoot) : OscConnector(oscRoot, true) {
	connect(&oscSerialPort, &QIODevice::readyRead, this, &SerialPortInterface::onOscDataReceived);
	connect(&oscSerialPort, &QSerialPort::errorOccurred, this, &SerialPortInterface::onOscErrorOccurred);
	connect(&oscReconnectTimer, &QTimer::timeout, this, &SerialPortInterface::onOscReconnect);
	oscReconnectTimer.setSingleShot(true);
	oscReconnectTimer.setInterval(1000);

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
		// printf("data: %s\n", receivedData.constData());
		SPDLOG_TRACE("Received OSC data: {:a}", spdlog::to_hex(receivedData));
		OscConnector::onOscDataReceived((const uint8_t*) receivedData.data(), receivedData.size());
	} while(oscSerialPort.bytesAvailable());
}

void SerialPortInterface::onOscErrorOccurred(QSerialPort::SerialPortError error) {
	if(error == QSerialPort::NoError)
		return;
	SPDLOG_ERROR("{}: error {}", oscSerialPort.portName().toStdString(), oscSerialPort.errorString().toStdString());
	if(oscSerialPort.isOpen())
		oscSerialPort.close();
	oscReconnectTimer.start();
}

void SerialPortInterface::onOscReconnect() {
	QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();
	bool portFound = false;
	for(const QSerialPortInfo& port : ports) {
		SPDLOG_INFO("available port: {}: {}, {}, {}, {}",
		            port.portName().toStdString(),
		            port.description().toStdString(),
		            port.manufacturer().toStdString(),
		            port.serialNumber().toStdString(),
		            port.systemLocation().toStdString());
		if(port.description().contains("DAMC STM32 Audio") || port.serialNumber().contains("DAMC_")) {
			oscSerialPort.setPort(port);
			portFound = true;
		}
	}

	if(portFound) {
		if(oscSerialPort.open(QIODevice::ReadWrite)) {
			SPDLOG_INFO("Opened: {}", oscSerialPort.portName().toStdString());
			oscSerialPort.clear();
			updateOscVariables();
		} else {
			SPDLOG_ERROR("Open failed: {}", oscSerialPort.errorString().toStdString());
			portFound = false;
		}
	}

	if(!portFound) {
		SPDLOG_INFO("Serial port DAMC STM32 Audio not available, waiting");
		oscReconnectTimer.start();
	}
}

void SerialPortInterface::sendOscData(const uint8_t* data, size_t size) {
	qint64 totalWritten = 0;
	do {
		qint64 byteWritten = oscSerialPort.write((const char*) data + totalWritten, size - totalWritten);
		if(byteWritten < 0) {
			break;
		}
		totalWritten += byteWritten;

		if(totalWritten >= (qint64) size) {
			break;
		}

		oscSerialPort.waitForBytesWritten(100);
	} while(1);
}
