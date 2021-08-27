#pragma once

#include "OscContainer.h"

template<typename T> class OscReadOnlyVariable : protected OscContainer {
public:
	using underlying_type = T;

	using OscContainer::getFullAddress;
	using OscContainer::getName;

	OscReadOnlyVariable(OscContainer* parent, std::string name, T initialValue = {});
	OscReadOnlyVariable(const OscReadOnlyVariable&) = delete;

	void set(T v);
	void setDefault(T v);
	void forceDefault(T v);

	T& get() { return value; }
	const T& get() const { return value; }
	bool isDefault() const { return isDefaultValue; }

	template<typename U = T> std::enable_if_t<std::is_same_v<U, std::string>, const char*> c_str();

	operator T() const { return value; }
	void dump() override { notifyOsc(); }

	using OscContainer::operator=;
	OscReadOnlyVariable& operator=(const T& v);
	OscReadOnlyVariable& operator=(const OscReadOnlyVariable<T>& v);

	template<typename U = T>
	std::enable_if_t<std::is_same_v<U, std::string>, OscReadOnlyVariable&> operator=(const char* v);
	bool operator==(const OscReadOnlyVariable<T>& other) { return value == other.value; }
	bool operator!=(const OscReadOnlyVariable<T>& other) { return !(*this == other); }

	std::string getAsString() const override;

	void setOscConverters(std::function<T(T)> convertToOsc, std::function<T(T)> convertFromOsc);

	void addCheckCallback(std::function<bool(T)> onChange);
	void setChangeCallback(std::function<void(T)> onChange);

	void callChangeCallbacks(T v);
	bool callCheckCallbacks(T v);

protected:
	void notifyOsc();

	T getToOsc() const;
	void setFromOsc(T value);

private:
	T value{};

	std::function<T(T)> convertToOsc;
	std::function<T(T)> convertFromOsc;
	std::vector<std::function<bool(T)>> checkCallbacks;
	std::vector<std::function<void(T)>> onChangeCallbacks;
	bool isDefaultValue;
};

template<typename T>
OscReadOnlyVariable<T>::OscReadOnlyVariable(OscContainer* parent, std::string name, T initialValue)
    : OscContainer(parent, name), value(initialValue), isDefaultValue(true) {
	if(notifyOscAtInit())
		notifyOsc();
}

template<typename T> void OscReadOnlyVariable<T>::set(T v) {
	if(value != v || isDefaultValue) {
		bool isDataValid = callCheckCallbacks(v);
		if(isDataValid) {
			isDefaultValue = false;
			value = v;
			callChangeCallbacks(v);
			notifyOsc();
			notifyValueChanged();
		} else {
			if constexpr(std::is_same_v<T, std::string>)
				printf("%s: refused value %s\n", getFullAddress().c_str(), v.c_str());
			else
				printf("%s: refused value %s\n", getFullAddress().c_str(), std::to_string(v).c_str());
		}
	}
}

template<typename T> void OscReadOnlyVariable<T>::setDefault(T v) {
	if(isDefaultValue)
		set(v);
}

template<typename T> void OscReadOnlyVariable<T>::forceDefault(T v) {
	set(v);
	isDefaultValue = true;
}

template<typename T>
template<typename U>
std::enable_if_t<std::is_same_v<U, std::string>, const char*> OscReadOnlyVariable<T>::c_str() {
	return value.c_str();
}

template<typename T> OscReadOnlyVariable<T>& OscReadOnlyVariable<T>::operator=(const T& v) {
	set(v);
	return *this;
}

template<typename T> OscReadOnlyVariable<T>& OscReadOnlyVariable<T>::operator=(const OscReadOnlyVariable<T>& v) {
	set(v.value);
	return *this;
}

template<typename T>
template<typename U>
std::enable_if_t<std::is_same_v<U, std::string>, OscReadOnlyVariable<T>&> OscReadOnlyVariable<T>::operator=(
    const char* v) {
	set(v);
	return *this;
}

template<typename T> std::string OscReadOnlyVariable<T>::getAsString() const {
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
		set(value);
	else
		set(convertFromOsc(value));
}
