#include "WavePlayInterface.h"
#include "WavePlayOutputInterface.h"
#include <QJsonDocument>
#include <QMessageBox>
#include <QNetworkDatagram>

static constexpr uint8_t SLIP_END = 0xC0;
static constexpr uint8_t SLIP_ESC = 0xDB;
static constexpr uint8_t SLIP_ESC_END = 0xDC;
static constexpr uint8_t SLIP_ESC_ESC = 0xDD;

WavePlayInterface::WavePlayInterface() : oscIsEscaping(false) {
	connect(&controlSocket, &QTcpSocket::readyRead, this, &WavePlayInterface::onDataReceived);
	connect(&controlSocket, &QTcpSocket::stateChanged, this, &WavePlayInterface::onConnectionStateChanged);

	onReconnect();

	connect(&oscSocket, &QIODevice::readyRead, this, &WavePlayInterface::onOscDataReceived);
	connect(&oscSocket, &QAbstractSocket::stateChanged, this, &WavePlayInterface::onOscConnectionStateChanged);

	onOscReconnect();
}

void WavePlayInterface::sendMessage(const QJsonObject& message) {
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

		if constexpr(std::is_same_v<decltype(oscSocket), QTcpSocket>) {
			receivedData = oscSocket.readAll();

			// Decode SLIP frame
			for(uint8_t c : qAsConst(receivedData)) {
				if(!oscIsEscaping) {
					if(c == SLIP_ESC) {
						oscIsEscaping = true;
						continue;
					} else if(c == SLIP_END) {
						if(!oscNetworkBuffer.empty()) {
							onOscPacketReceived(oscNetworkBuffer.data(), oscNetworkBuffer.size());
							oscNetworkBuffer.clear();
						}
						continue;
					}
					// else this is a regular character
				} else {
					if(c == SLIP_ESC_END) {
						c = SLIP_END;
					} else if(c == SLIP_ESC_ESC) {
						c = SLIP_ESC;
					}
					// else this is an error, escaped character doesn't need to be escaped
				}
				oscIsEscaping = false;
				oscNetworkBuffer.push_back(c);
			}
		} else {
			receivedData = oscSocket.receiveDatagram().data();
			onOscPacketReceived((const uint8_t*) receivedData.data(), receivedData.size());
		}
	} while(oscSocket.bytesAvailable());
}

void WavePlayInterface::onOscConnectionStateChanged(QAbstractSocket::SocketState state) {
	if(state == QAbstractSocket::UnconnectedState) {
		oscReconnectTimer.singleShot(1000, this, &WavePlayInterface::onOscReconnect);
	}
}

void WavePlayInterface::onOscReconnect() {
	oscIsEscaping = false;
	if constexpr(std::is_same_v<decltype(oscSocket), QTcpSocket>) {
		oscSocket.connectToHost("127.0.0.1", 2308);
	} else {
		oscSocket.bind(10000, QAbstractSocket::ReuseAddressHint);
	}
}

void WavePlayInterface::sendNextMessage(const uint8_t* data, size_t size) {
	if constexpr(std::is_same_v<decltype(oscSocket), QTcpSocket>) {
		if(oscSocket.state() != QTcpSocket::ConnectedState) {
			onOscReconnect();
			return;
		}

		oscOutputNetworkBuffer.clear();

		// Encode SLIP frame with double-END variant (one at the start, one at the end)
		oscOutputNetworkBuffer.push_back(SLIP_END);

		for(size_t i = 0; i < size; i++) {
			const uint8_t c = data[i];

			if(c == SLIP_END) {
				oscOutputNetworkBuffer.push_back(SLIP_ESC);
				oscOutputNetworkBuffer.push_back(SLIP_ESC_END);
			} else if(c == SLIP_ESC) {
				oscOutputNetworkBuffer.push_back(SLIP_ESC);
				oscOutputNetworkBuffer.push_back(SLIP_ESC_ESC);
			} else {
				oscOutputNetworkBuffer.push_back(c);
			}
		}

		oscOutputNetworkBuffer.push_back(SLIP_END);

		oscSocket.write(oscOutputNetworkBuffer);
	} else {
		oscSocket.writeDatagram((const char*) data, size, QHostAddress::LocalHost, 2307);
	}

	onMessageSent();
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
