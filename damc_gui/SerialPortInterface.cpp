#include "SerialPortInterface.h"
#include <QMessageBox>
#include <QSerialPortInfo>
#include <spdlog/fmt/bin_to_hex.h>
#include <spdlog/spdlog.h>

SerialPortInterface::SerialPortInterface(OscRoot* oscRoot) : OscConnector(oscRoot, true) {
	connect(&oscSerialPort, &QIODevice::readyRead, this, &SerialPortInterface::onOscDataReceived);
	connect(&oscSerialPort, &QSerialPort::errorOccurred, this, &SerialPortInterface::onOscErrorOccurred);

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
		SPDLOG_DEBUG("Received OSC data: {:a}", spdlog::to_hex(receivedData));
		OscConnector::onOscDataReceived((const uint8_t*) receivedData.data(), receivedData.size());
	} while(oscSerialPort.bytesAvailable());
}

void SerialPortInterface::onOscErrorOccurred(QSerialPort::SerialPortError error) {
	if(error == QSerialPort::NoError)
		return;
	SPDLOG_ERROR("{}: error {}", oscSerialPort.portName().toStdString(), oscSerialPort.errorString().toStdString());
	oscSerialPort.close();
	oscReconnectTimer.singleShot(1000, this, &SerialPortInterface::onOscReconnect);
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
			updateOscVariables();
		} else {
			SPDLOG_ERROR("Open failed: {}", oscSerialPort.errorString().toStdString());
			portFound = false;
		}
	}

	if(!portFound) {
		SPDLOG_INFO("Serial port DAMC STM32 Audio not available, waiting");
		oscReconnectTimer.singleShot(1000, this, &SerialPortInterface::onOscReconnect);
	}
}

void SerialPortInterface::sendOscData(const uint8_t* data, size_t size) {
	oscSerialPort.write((const char*) data, size);
}
