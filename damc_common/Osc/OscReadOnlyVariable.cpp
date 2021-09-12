#include "OscReadOnlyVariable.h"
#include "OscRoot.h"

EXPLICIT_INSTANCIATE_OSC_VARIABLE(template, OscReadOnlyVariable)

template<typename T>
OscReadOnlyVariable<T>::OscReadOnlyVariable(OscContainer* parent, std::string name, T initialValue)
    : OscContainer(parent, name), value(initialValue), isDefaultValue(true) {
	if(getRoot()->isOscValueAuthority())
		notifyOsc();
}

template<typename T> void OscReadOnlyVariable<T>::set(T v, bool fromOsc) {
	if(value != v || isDefaultValue) {
		bool isDataValid = callCheckCallbacks(v);
		if(isDataValid) {
			isDefaultValue = false;
			value = v;
			callChangeCallbacks(v);
			if(!fromOsc || getRoot()->isOscValueAuthority())
				notifyOsc();
			getRoot()->notifyValueChanged();
		} else {
			if constexpr(std::is_same_v<T, std::string>)
				printf("%s: refused value %s\n", getFullAddress().c_str(), v.c_str());
			else
				printf("%s: refused value %s\n", getFullAddress().c_str(), std::to_string(v).c_str());

			if(fromOsc) {
				// Ensure the client that set this is notified that the value didn't changed
				notifyOsc();
			}
		}
	}
}

template<typename T> void OscReadOnlyVariable<T>::setDefault(T v) {
	if(isDefaultValue) {
		isDefaultValue = false;  // Only notify if the value is different
		set(v);
		isDefaultValue = true;
	}
}

template<typename T> void OscReadOnlyVariable<T>::forceDefault(T v) {
	isDefaultValue = false;  // Only notify if the value is different
	set(v);
	isDefaultValue = true;
}

template<typename T> OscReadOnlyVariable<T>& OscReadOnlyVariable<T>::operator=(const T& v) {
	set(v);
	return *this;
}

template<typename T> OscReadOnlyVariable<T>& OscReadOnlyVariable<T>::operator=(const OscReadOnlyVariable<T>& v) {
	set(v.value);
	return *this;
}

template<typename T> std::string OscReadOnlyVariable<T>::getAsString() const {
	if(isDefaultValue)
		return {};

	if constexpr(std::is_same_v<T, std::string>) {
		return "\"" + getToOsc() + "\"";
	} else {
		return std::to_string(getToOsc());
	}
}

template<typename T>
void OscReadOnlyVariable<T>::setOscConverters(std::function<T(T)> convertToOsc, std::function<T(T)> convertFromOsc) {
	this->convertToOsc = convertToOsc;
	this->convertFromOsc = convertFromOsc;
}

template<typename T> void OscReadOnlyVariable<T>::addCheckCallback(std::function<bool(T)> checkCallback) {
	this->checkCallbacks.push_back(checkCallback);
	checkCallback(this->get());
}

template<typename T> void OscReadOnlyVariable<T>::setChangeCallback(std::function<void(T)> onChange) {
	this->onChangeCallbacks.push_back(onChange);
	onChange(this->get());
}

template<typename T> void OscReadOnlyVariable<T>::callChangeCallbacks(T v) {
	for(auto& callback : onChangeCallbacks) {
		callback(v);
	}
}

template<typename T> bool OscReadOnlyVariable<T>::callCheckCallbacks(T v) {
	bool isDataValid = true;
	for(auto& callback : checkCallbacks) {
		isDataValid = isDataValid && callback(v);
	}
	return isDataValid;
}

template<typename T> void OscReadOnlyVariable<T>::notifyOsc() {
	OscArgument valueToSend = getToOsc();
	sendMessage(&valueToSend, 1);
}

template<typename T> T OscReadOnlyVariable<T>::getToOsc() const {
	if(!convertToOsc)
		return get();
	else
		return convertToOsc(get());
}

template<typename T> void OscReadOnlyVariable<T>::setFromOsc(T value) {
	if(!convertFromOsc)
		set(value, true);
	else
		set(convertFromOsc(value), true);
}
