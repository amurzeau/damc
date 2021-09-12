#pragma once

#include "OscContainer.h"
#include <functional>
#include <stdint.h>
#include <string>
#include <vector>

#define EXPLICIT_INSTANCIATE_OSC_VARIABLE(prefix_, template_name_) \
	prefix_ class template_name_<bool>; \
	prefix_ class template_name_<int32_t>; \
	prefix_ class template_name_<float>; \
	prefix_ class template_name_<std::string>;

template<typename T> class OscReadOnlyVariable : protected OscContainer {
public:
	using underlying_type = T;

	using OscContainer::getFullAddress;
	using OscContainer::getName;

	OscReadOnlyVariable(OscContainer* parent, std::string name, T initialValue = {});
	OscReadOnlyVariable(const OscReadOnlyVariable&) = delete;

	void set(T v, bool fromOsc = false);
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
	std::enable_if_t<std::is_same_v<U, std::string>, OscReadOnlyVariable<T>&> operator=(const char* v);
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

EXPLICIT_INSTANCIATE_OSC_VARIABLE(extern template, OscReadOnlyVariable)

template<typename T>
template<typename U>
std::enable_if_t<std::is_same_v<U, std::string>, OscReadOnlyVariable<T>&> OscReadOnlyVariable<T>::operator=(
    const char* v) {
	set(v);
	return *this;
}

template<typename T>
template<typename U>
std::enable_if_t<std::is_same_v<U, std::string>, const char*> OscReadOnlyVariable<T>::c_str() {
	return value.c_str();
}
