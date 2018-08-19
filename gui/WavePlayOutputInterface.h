#ifndef WAVEPLAYOUTPUTINTERFACE_H
#define WAVEPLAYOUTPUTINTERFACE_H

#include "MessageInterface.h"
#include <QObject>

class WavePlayInterface;

class WavePlayOutputInterface : public QObject {
	Q_OBJECT
public:
	WavePlayOutputInterface();
	~WavePlayOutputInterface();

	void setInterface(int index, WavePlayInterface* interface);
	void sendMessage(control_message_t message);

	void messageReiceved(notification_message_t message);

signals:
	void onMessage(notification_message_t message);

private:
	int outputInstance;
	WavePlayInterface* interface;
};

#endif  // WAVEPLAYOUTPUTINTERFACE_H
