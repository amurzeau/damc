#pragma once

#include "OscGenericArray.h"
#include "OscVariable.h"

template<typename T> class OscArray : public OscGenericArray<OscVariable<T>> {
public:
	OscArray(OscContainer* parent, std::string name);

	void setOscConverters(std::function<T(T)> convertToOsc, std::function<T(T)> convertFromOsc);
	void setChangeCallback(std::function<void(T)> onChange) { this->onChange = onChange; }

	void setAddCallback(std::function<void(T)> onAdd) { this->onAdd = onAdd; }
	void setRemoveCallback(std::function<void()> onRemove) { this->onRemove = onRemove; }

protected:
	void initializeItem(OscVariable<T>* item) override;

private:
	std::function<T(T)> convertToOsc;
	std::function<T(T)> convertFromOsc;
	std::function<void(T)> onChange;

	std::function<void(T)> onAdd;
	std::function<void()> onRemove;
};

template<typename T>
OscArray<T>::OscArray(OscContainer* parent, std::string name) : OscGenericArray<OscVariable<T>>(parent, name) {
	this->oscAddEndpoint.setCallback([this](auto&&) {
		printf("Adding item to %s\n", this->getFullAddress().c_str());
		T v{};
		this->push_back(v);
		this->onAdd(v);
	});
	this->oscRemoveEndpoint.setCallback([this](auto&&) {
		printf("Removing last item from %s\n", this->getFullAddress().c_str());
		this->onRemove();
		this->pop_back();
	});
}

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
