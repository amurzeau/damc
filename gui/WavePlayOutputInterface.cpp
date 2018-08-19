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

void WavePlayOutputInterface::sendMessage(control_message_t message) {
	message.outputInstance = outputInstance;
	interface->sendMessage(message);
}

void WavePlayOutputInterface::messageReiceved(notification_message_t message) {
	emit onMessage(message);
}
