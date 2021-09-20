#pragma once

#include <OscRoot.h>
#include <QObject>
#include <QTcpSocket>
#include <QTimer>
#include <stdint.h>

class WavePlayInterface : public QObject, public OscConnector {
	Q_OBJECT
public:
	WavePlayInterface(OscRoot* oscRoot);

	void updateOscVariables();

protected slots:
	void onOscDataReceived();
	void onOscConnectionStateChanged(QAbstractSocket::SocketState state);
	void onOscReconnect();

protected:
	void sendOscData(const uint8_t* data, size_t size) override;

private:
	QTcpSocket oscSocket;
	QTimer oscReconnectTimer;
};
