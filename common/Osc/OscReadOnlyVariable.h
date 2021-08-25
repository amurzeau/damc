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

	T& get() { return value; }
	const T& get() const { return value; }

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

	void setChangeCallback(std::function<void(T)> onChange);

protected:
	template<class U> friend class OscGenericArray;

	void notifyOsc();

	T getToOsc() const;
	void setFromOsc(T value);

private:
	T value{};

	std::function<T(T)> convertToOsc;
	std::function<T(T)> convertFromOsc;
	std::function<void(T)> onChange;
};

template<typename T>
OscReadOnlyVariable<T>::OscReadOnlyVariable(OscContainer* parent, std::string name, T initialValue)
    : OscContainer(parent, name), value(initialValue) {
	if(notifyOscAtInit())
		notifyOsc();
}

template<typename T> void OscReadOnlyVariable<T>::set(T v) {
	if(value != v) {
		value = v;
		notifyOsc();
		if(onChange) {
			onChange(this->get());
		}
	}
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

template<typename T> void OscReadOnlyVariable<T>::setChangeCallback(std::function<void(T)> onChange) {
	this->onChange = onChange;
	onChange(this->get());
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
