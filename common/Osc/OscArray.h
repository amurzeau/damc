#pragma once

#include "OscGenericArray.h"
#include "OscVariable.h"

template<typename T> class OscArray : public OscGenericArray<OscVariable<T>> {
public:
	OscArray(OscContainer* parent, std::string name);

	void setOscConverters(std::function<T(T)> convertToOsc, std::function<T(T)> convertFromOsc);
	void setChangeCallback(std::function<void(T)> onChange) { this->onChange = onChange; }

protected:
	void initializeItem(OscVariable<T>* item) override;

private:
	std::function<T(T)> convertToOsc;
	std::function<T(T)> convertFromOsc;
	std::function<void(T)> onChange;
};

template<typename T>
OscArray<T>::OscArray(OscContainer* parent, std::string name) : OscGenericArray<OscVariable<T>>(parent, name) {}

template<typename T>
void OscArray<T>::setOscConverters(std::function<T(T)> convertToOsc, std::function<T(T)> convertFromOsc) {
	this->convertToOsc = convertToOsc;
	this->convertFromOsc = convertFromOsc;
}

template<typename T> void OscArray<T>::initializeItem(OscVariable<T>* item) {
	if(convertToOsc || convertFromOsc) {
		item->setOscConverters(convertToOsc, convertFromOsc);
	}
	if(onChange) {
		item->setChangeCallback(onChange);
	}
}
