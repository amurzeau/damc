#pragma once

#include <OscRoot.h>
#include <QObject>
#include <QSerialPort>
#include <QTimer>
#include <stdint.h>

class SerialPortInterface : public QObject, public OscConnector {
	Q_OBJECT
public:
	SerialPortInterface(OscRoot* oscRoot, QString portName);

	void updateOscVariables();

protected slots:
	void onOscDataReceived();
	void onOscReconnect();

protected:
	void sendOscData(const uint8_t* data, size_t size) override;

private:
	QSerialPort oscSerialPort;
	QTimer oscReconnectTimer;
};
