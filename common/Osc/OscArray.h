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

EXPLICIT_INSTANCIATE_OSC_VARIABLE(extern template, OscArray)
