#ifndef WAVEPLAYINTERFACE_H
#define WAVEPLAYINTERFACE_H

#include "OscRoot.h"
#include <QJsonObject>
#include <QObject>
#include <QTcpSocket>
#include <QTimer>
#include <stdint.h>
#include <unordered_map>
#include <vector>

class WavePlayOutputInterface;

class WavePlayInterface : public QObject, public OscRoot {
	Q_OBJECT
public:
	WavePlayInterface();

	void sendMessage(const QJsonObject& message);
	void addOutputInterface(int index, WavePlayOutputInterface* outputInterface) {
		outputInterfaces[index] = outputInterface;
	}
	void removeOutputInterface(int index) { outputInterfaces.erase(index); }

protected slots:
	void onDataReceived();
	void onConnectionStateChanged(QAbstractSocket::SocketState state);
	void onReconnect();

	void onOscDataReceived();
	void onOscConnectionStateChanged(QAbstractSocket::SocketState state);
	void onOscReconnect();

protected:
	void sendNextMessage(const uint8_t* data, size_t size) override;
	void onPacketReceived(const void* data, size_t size);

private:
	QTcpSocket oscSocket;
	QTimer oscReconnectTimer;
	std::vector<uint8_t> oscNetworkBuffer;
	QByteArray oscOutputNetworkBuffer;
	bool oscIsEscaping;

	QTcpSocket controlSocket;
	QTimer reconnectTimer;
	std::vector<uint8_t> networkBuffer;
	std::unordered_map<int, WavePlayOutputInterface*> outputInterfaces;
};

#endif  // WAVEPLAYINTERFACE_H
