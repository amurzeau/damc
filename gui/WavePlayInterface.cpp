#include "WavePlayInterface.h"
#include "WavePlayOutputInterface.h"
#include <QMessageBox>

WavePlayInterface::WavePlayInterface() {
	connect(&controlSocket, SIGNAL(readyRead()), this, SLOT(onDataReceived()));
	connect(&controlSocket,
	        SIGNAL(stateChanged(QAbstractSocket::SocketState)),
	        this,
	        SLOT(onConnectionStateChanged(QAbstractSocket::SocketState)));

	controlSocket.connectToHost("127.0.0.1", 2306);
}

void WavePlayInterface::sendMessage(struct control_message_t message) {
	qDebug("Reveived data: %d, %d", (int) controlSocket.bytesAvailable(), (int) networkBuffer.size());
	int sizeData = sizeof(message);
	if(controlSocket.state() != QTcpSocket::ConnectedState)
		controlSocket.connectToHost("127.0.0.1", 2306);

	controlSocket.write((const char*) &sizeData, sizeof(sizeData));
	controlSocket.write((const char*) &message, sizeof(message));
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
		reconnectTimer.singleShot(1000, this, SLOT(onReconnect()));
}

void WavePlayInterface::onReconnect() {
	controlSocket.connectToHost("127.0.0.1", 2306);
}

void WavePlayInterface::onPacketReceived(const void* data, size_t) {
	const notification_message_t* message = reinterpret_cast<const notification_message_t*>(data);
	auto it = outputInterfaces.find(message->outputInstance);
	if(it != outputInterfaces.end()) {
		it->second->messageReiceved(*message);
	} else {
		qDebug("Bad instance %d", message->outputInstance);
	}
}
