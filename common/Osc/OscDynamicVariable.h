#pragma once

#include "OscContainer.h"

template<typename T> class OscDynamicVariable : public OscContainer {
public:
	using underlying_type = T;

	OscDynamicVariable(OscContainer* parent, std::string name);
	OscDynamicVariable(const OscDynamicVariable&) = delete;

	std::vector<T> get();
	void dump() override { notifyOsc(); }

	std::string getAsString() const override;

	void setReadCallback(std::function<std::vector<T>()> onReadCallback);

protected:
	void notifyOsc();

private:
	std::function<std::vector<T>()> onReadCallback;
};

template<typename T>
OscDynamicVariable<T>::OscDynamicVariable(OscContainer* parent, std::string name) : OscContainer(parent, name) {}

template<typename T> std::vector<T> OscDynamicVariable<T>::get() {
	if(onReadCallback)
		return onReadCallback();
	else
		return std::vector<T>{};
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

	valueToSend.reserve(values.size());
	for(auto& v : values) {
		valueToSend.push_back(v);
	}
	sendMessage(&valueToSend[0], valueToSend.size());
}
