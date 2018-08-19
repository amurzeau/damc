#ifndef WAVEPLAYINTERFACE_H
#define WAVEPLAYINTERFACE_H

#include "MessageInterface.h"
#include <QObject>
#include <QTcpSocket>
#include <QTimer>
#include <stdint.h>
#include <unordered_map>
#include <vector>

class WavePlayOutputInterface;

class WavePlayInterface : public QObject {
	Q_OBJECT
public:
	WavePlayInterface();

	void sendMessage(struct control_message_t message);
	void addOutputInterface(int index, WavePlayOutputInterface* outputInterface) {
		outputInterfaces[index] = outputInterface;
	}
	void removeOutputInterface(int index) { outputInterfaces.erase(index); }

protected slots:
	void onDataReceived();
	void onConnectionStateChanged(QAbstractSocket::SocketState state);
	void onReconnect();

protected:
	void onPacketReceived(const void* data, size_t size);

private:
	QTcpSocket controlSocket;
	QTimer reconnectTimer;
	std::vector<uint8_t> networkBuffer;
	std::unordered_map<int, WavePlayOutputInterface*> outputInterfaces;
};

#endif  // WAVEPLAYINTERFACE_H
