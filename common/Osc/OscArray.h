#pragma once

#include "OscGenericArray.h"
#include "OscVariable.h"

template<typename T> class OscArray : public OscGenericArray<OscVariable<T>> {
public:
	OscArray(OscContainer* parent, std::string name);

	void setOscConverters(std::function<T(T)> convertToOsc, std::function<T(T)> convertFromOsc);
	void setChangeCallback(std::function<void(T)> onChangeCallbacks);

protected:
	void initializeItem(OscVariable<T>* item) override;

private:
	std::function<T(T)> convertToOsc;
	std::function<T(T)> convertFromOsc;
	std::vector<std::function<void(T)>> onChangeCallbacks;
};

template<typename T>
OscArray<T>::OscArray(OscContainer* parent, std::string name) : OscGenericArray<OscVariable<T>>(parent, name) {
	this->setFactory([](OscContainer* parent, int name) { return new OscVariable<T>(parent, std::to_string(name)); });
}

template<typename T>
void OscArray<T>::setOscConverters(std::function<T(T)> convertToOsc, std::function<T(T)> convertFromOsc) {
	this->convertToOsc = convertToOsc;
	this->convertFromOsc = convertFromOsc;
}

template<typename T> void OscArray<T>::setChangeCallback(std::function<void(T)> onChangeCallbacks) {
	this->onChangeCallbacks.push_back(onChangeCallbacks);
}

template<typename T> void OscArray<T>::initializeItem(OscVariable<T>* item) {
	if(convertToOsc || convertFromOsc) {
		item->setOscConverters(convertToOsc, convertFromOsc);
	}
	for(auto& callback : onChangeCallbacks) {
		item->setChangeCallback(callback);
	}
}
