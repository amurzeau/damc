#include "WavePlayInterface.h"
#include <QMessageBox>

WavePlayInterface::WavePlayInterface(OscRoot* oscRoot, QString ip, uint32_t port)
    : OscConnector(oscRoot, true), ip(ip), port(port) {
	connect(&oscSocket, &QIODevice::readyRead, this, &WavePlayInterface::onOscDataReceived);
	connect(&oscSocket, &QAbstractSocket::stateChanged, this, &WavePlayInterface::onOscConnectionStateChanged);

	onOscReconnect();
}

void WavePlayInterface::updateOscVariables() {
	getOscRoot()->sendMessage("/**/dump", nullptr, 0);
}

void WavePlayInterface::onOscDataReceived() {
	do {
		// QByteArray receivedData = oscSocket.readAll();
		QByteArray receivedData;

#if 1
		receivedData = oscSocket.readAll();
#else
		receivedData = oscSocket.receiveDatagram().data();
		oscSocket.writeDatagram(receivedData.data(), receivedData.size(), QHostAddress::LocalHost, 10001);
#endif
		OscConnector::onOscDataReceived((const uint8_t*) receivedData.data(), receivedData.size());
	} while(oscSocket.bytesAvailable());
}

void WavePlayInterface::onOscConnectionStateChanged(QAbstractSocket::SocketState state) {
	if(state == QAbstractSocket::UnconnectedState) {
		oscReconnectTimer.singleShot(1000, this, &WavePlayInterface::onOscReconnect);
	} else if(state == QAbstractSocket::ConnectedState) {
		updateOscVariables();
	}
}

void WavePlayInterface::onOscReconnect() {
#if 1
	oscSocket.connectToHost(ip, port);
#else
	oscSocket.bind(10000, QAbstractSocket::ReuseAddressHint);
#endif
}

void WavePlayInterface::sendOscData(const uint8_t* data, size_t size) {
#if 1
	if(oscSocket.state() != QTcpSocket::ConnectedState) {
		onOscReconnect();
		return;
	}

	oscSocket.write((const char*) data, size);
#else
	oscSocket.writeDatagram((const char*) data, size, QHostAddress::LocalHost, 2307);
	oscSocket.writeDatagram((const char*) data, size, QHostAddress::LocalHost, 10001);
#endif
}
