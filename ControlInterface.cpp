#include "ControlInterface.h"
#include "gui/MessageInterface.h"

#include <string.h>

ControlInterface::ControlInterface() : numChannels(2), numEq(4) {}

ControlInterface::~ControlInterface() {}

int ControlInterface::init(const char* controlIp, int controlPort) {
	controlServer.init(controlIp, controlPort, &messageProcessorStatic, &onNewClientStatic, this);

	return 0;
}

void ControlInterface::addLocalOutput() {
	outputs.emplace_back(new OutputInstance);
	OutputInstance* output = outputs.back().get();
	output->init(&controlServer, OutputInstance::Local, outputs.size() - 1, numChannels, numEq, nullptr, 0);
}

void ControlInterface::addRemoteOutput(const char* ip, int port) {
	outputs.emplace_back(new OutputInstance);
	OutputInstance* output = outputs.back().get();
	output->init(&controlServer, OutputInstance::Remote, outputs.size() - 1, numChannels, numEq, ip, port);
}

void ControlInterface::run() {
	controlServer.run();
}

void ControlInterface::onNewClientStatic(void* arg, ControlClient* client) {
	ControlInterface* thisInstance = (ControlInterface*) arg;
	thisInstance->onNewClient(client);
}

void ControlInterface::onNewClient(ControlClient* client) {
	struct notification_message_t message;
	message.opcode = notification_message_t::op_state;
	message.outputInstance = -1;
	message.data.state.numOutputInstances = outputs.size();
	message.data.state.numChannels = numChannels;
	message.data.state.numEq = numEq;

	client->sendMessage(&message, sizeof(message));

	for(std::unique_ptr<OutputInstance>& output : outputs)
		output->sendFilterParameters(client);
}

void ControlInterface::messageProcessorStatic(void* arg, const void* data, size_t size) {
	ControlInterface* thisInstance = (ControlInterface*) arg;
	thisInstance->messageProcessor(data, size);
}

void ControlInterface::messageProcessor(const void* data, size_t size) {
	const control_message_t* message = (control_message_t*) data;

	if(message->outputInstance < (int) outputs.size())
		outputs[message->outputInstance]->messageProcessor(data, size);
}
