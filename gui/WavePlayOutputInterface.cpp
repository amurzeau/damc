#include "WavePlayOutputInterface.h"
#include "WavePlayInterface.h"

WavePlayOutputInterface::WavePlayOutputInterface() : interface(nullptr) {}

WavePlayOutputInterface::~WavePlayOutputInterface() {
	if(this->interface)
		this->interface->removeOutputInterface(this->outputInstance);
	this->interface = nullptr;
}

void WavePlayOutputInterface::setInterface(int index, WavePlayInterface* interface) {
	this->interface = interface;
	this->outputInstance = index;
	interface->addOutputInterface(index, this);
}

void WavePlayOutputInterface::sendMessage(const QJsonObject& message) {
	QJsonObject json = message;
	json["instance"] = outputInstance;
	interface->sendMessage(json);
}

void WavePlayOutputInterface::messageReiceved(const QJsonObject& message) {
	emit onMessage(message);
}
