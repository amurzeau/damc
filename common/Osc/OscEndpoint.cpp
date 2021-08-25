#include "OscEndpoint.h"

void OscEndpoint::setCallback(std::function<void(const std::vector<OscArgument>&)> onExecute) {
	this->onExecute = std::move(onExecute);
}

void OscEndpoint::execute(const std::vector<OscArgument>& arguments) {
	if(onExecute)
		onExecute(arguments);
}
