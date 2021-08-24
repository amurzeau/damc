#include "WavePlayInterface.h"
#include "WavePlayOutputInterface.h"
#include <QJsonDocument>
#include <QMessageBox>
#include <QNetworkDatagram>

WavePlayInterface::WavePlayInterface(OscRoot* oscRoot) : OscConnector(oscRoot, true) {
	connect(&controlSocket, &QTcpSocket::readyRead, this, &WavePlayInterface::onDataReceived);
	connect(&controlSocket, &QTcpSocket::stateChanged, this, &WavePlayInterface::onConnectionStateChanged);

	onReconnect();

	connect(&oscSocket, &QIODevice::readyRead, this, &WavePlayInterface::onOscDataReceived);
	connect(&oscSocket, &QAbstractSocket::stateChanged, this, &WavePlayInterface::onOscConnectionStateChanged);

	onOscReconnect();
}

void WavePlayInterface::updateOscVariables() {
	getOscRoot()->sendMessage("/**/dump", nullptr, 0);

	OscArgument arg = true;
	getOscRoot()->sendMessage("/strip/*/enable_peak", &arg, 1);
}

void WavePlayInterface::sendMessage(const QJsonObject& message) {
	return;

	QByteArray data = QJsonDocument(message).toJson(QJsonDocument::Compact);
	int sizeData = data.size();

	qDebug("Sending data: %s", data.constData());
	if(controlSocket.state() != QTcpSocket::ConnectedState)
		controlSocket.connectToHost("127.0.0.1", 2306);

	controlSocket.write((const char*) &sizeData, sizeof(sizeData));
	controlSocket.write(data.constData(), sizeData);
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
	oscSocket.connectToHost("127.0.0.1", 2308);
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

void WavePlayInterface::onDataReceived() {
	do {
		QByteArray receivedData = controlSocket.readAll();
		networkBuffer.insert(networkBuffer.end(), receivedData.begin(), receivedData.end());

		do {
			size_t currentMessageSize;

			if(networkBuffer.size() >= sizeof(int)) {
				currentMessageSize = *reinterpret_cast<int*>(networkBuffer.data());
			} else {
				// not enough data to get size
				break;
			}

			if(currentMessageSize > 1000) {
				qDebug("large currentMessageSize %d", (int) currentMessageSize);
			}

			// not enough data to have full packet
			if(networkBuffer.size() < currentMessageSize + sizeof(int))
				break;

			onPacketReceived(networkBuffer.data() + sizeof(int), currentMessageSize);
			networkBuffer.erase(networkBuffer.begin(), networkBuffer.begin() + currentMessageSize + sizeof(int));

		} while(!networkBuffer.empty());
	} while(controlSocket.bytesAvailable());
}

void WavePlayInterface::onConnectionStateChanged(QAbstractSocket::SocketState state) {
	if(state == QAbstractSocket::UnconnectedState)
		reconnectTimer.singleShot(1000, this, &WavePlayInterface::onReconnect);
}

void WavePlayInterface::onReconnect() {
	controlSocket.connectToHost("127.0.0.1", 2306);
}

void WavePlayInterface::onPacketReceived(const void* data, size_t size) {
	return;
	QJsonParseError error;
	QJsonDocument jsonDocument = QJsonDocument::fromJson(QByteArray::fromRawData((const char*) data, size), &error);
	if(jsonDocument.isNull()) {
		qDebug("Null document: %s", error.errorString().toUtf8().constData());
	}

	QJsonObject json = jsonDocument.object();

	// qDebug("Received message %s", QJsonDocument(json).toJson(QJsonDocument::Indented).constData());

	int outputInstance = json["instance"].toInt();

	auto it = outputInterfaces.find(outputInstance);
	if(it != outputInterfaces.end()) {
		it->second->messageReiceved(json);
	} else {
		qDebug("Bad instance %d", outputInstance);
	}
}
