#include "OscDynamicVariable.h"
#include <spdlog/spdlog.h>

EXPLICIT_INSTANCIATE_OSC_VARIABLE(template, OscDynamicVariable)

template<typename T>
OscDynamicVariable<T>::OscDynamicVariable(OscContainer* parent, std::string name) : OscContainer(parent, name) {}

template<typename T> std::vector<T> OscDynamicVariable<T>::get() {
	if(onReadCallback) {
		return onReadCallback();
	} else {
		return std::vector<T>{};
	}
}

template<typename T> std::string OscDynamicVariable<T>::getAsString() const {
	return {};
}

template<typename T> void OscDynamicVariable<T>::setReadCallback(std::function<std::vector<T>()> onReadCallback) {
	this->onReadCallback = onReadCallback;
}

template<typename T> void OscDynamicVariable<T>::notifyOsc() {
	std::vector<OscArgument> valueToSend;
	auto values = get();

	SPDLOG_TRACE("{} = ", getFullAddress());

	valueToSend.reserve(values.size());
	for(const auto& v : values) {
		SPDLOG_TRACE(" - {} ", v);
		valueToSend.push_back(v);
	}

	sendMessage(&valueToSend[0], valueToSend.size());
}
