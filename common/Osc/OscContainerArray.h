#pragma once

#include "OscGenericArray.h"

template<typename T> class OscContainerArray : public OscGenericArray<T> {
public:
	OscContainerArray(OscContainer* parent, std::string name);

	void setAddCallback(std::function<void()> onAdd) { this->onAdd = onAdd; }
	void setRemoveCallback(std::function<void()> onRemove) { this->onRemove = onRemove; }

private:
	std::function<void()> onAdd;
	std::function<void()> onRemove;
};

template<typename T>
OscContainerArray<T>::OscContainerArray(OscContainer* parent, std::string name) : OscGenericArray<T>(parent, name) {
	this->oscAddEndpoint.setCallback([this](auto&&) {
		printf("Adding item to %s\n", this->getFullAddress().c_str());
		this->push_back();
		if(this->onAdd)
			this->onAdd();
	});
	this->oscRemoveEndpoint.setCallback([this](auto&&) {
		printf("Removing last item from %s\n", this->getFullAddress().c_str());
		if(this->onRemove)
			this->onRemove();
		this->pop_back();
	});
}
