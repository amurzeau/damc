#pragma once

#include "OscGenericArray.h"

template<typename T> class OscContainerArray : public OscGenericArray<T> {
public:
	OscContainerArray(OscContainer* parent, std::string name);
};

template<typename T>
OscContainerArray<T>::OscContainerArray(OscContainer* parent, std::string name) : OscGenericArray<T>(parent, name) {}
