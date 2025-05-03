#pragma once

#include <OscRoot.h>
#include <QObject>
#include <QSerialPort>
#include <QTimer>
#include <stdint.h>

class SerialPortInterface : public QObject, public OscConnector {
	Q_OBJECT
public:
	SerialPortInterface(OscRoot* oscRoot);

	void updateOscVariables();

protected slots:
	void onOscDataReceived();
	void onOscErrorOccurred(QSerialPort::SerialPortError error);
	void onOscReconnect();

	void onCheckSerialPortAlive();

protected:
	void sendOscData(const uint8_t* data, size_t size) override;

private:
	QSerialPort oscSerialPort;
	QTimer oscReconnectTimer;

	// Workaround https://bugreports.qt.io/browse/QTBUG-130561
	QTimer checkSerialPortAlive;
	bool dataReceivedSinceLastCheck;
};
