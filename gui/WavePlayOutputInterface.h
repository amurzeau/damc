#ifndef WAVEPLAYOUTPUTINTERFACE_H
#define WAVEPLAYOUTPUTINTERFACE_H

#include <QJsonObject>
#include <QObject>

class WavePlayInterface;

class WavePlayOutputInterface : public QObject {
	Q_OBJECT
public:
	WavePlayOutputInterface();
	~WavePlayOutputInterface();

	void setInterface(int index, WavePlayInterface* interface);
	void sendMessage(const QJsonObject& message);

	void messageReiceved(const QJsonObject& message);

signals:
	void onMessage(QJsonObject message);

private:
	int outputInstance;
	WavePlayInterface* interface;
};

#endif  // WAVEPLAYOUTPUTINTERFACE_H
