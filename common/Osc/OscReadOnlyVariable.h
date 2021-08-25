#pragma once

#include "OscContainer.h"

template<typename T> class OscReadOnlyVariable : protected OscContainer {
public:
	using underlying_type = T;

	using OscContainer::getFullAddress;
	using OscContainer::getName;

	OscReadOnlyVariable(OscContainer* parent, std::string name, T initialValue = {}) noexcept
	    : OscContainer(parent, name), value(initialValue) {
		if(notifyOscAtInit())
			notifyOsc();
	}
	OscReadOnlyVariable(const OscReadOnlyVariable&) = delete;

	void set(T v) {
		if(value != v) {
			value = v;
			notifyOsc();
			if(onChange) {
				onChange(this->get());
			}
		}
	}

	T& get() { return value; }
	const T& get() const { return value; }

	template<typename U = T> std::enable_if_t<std::is_same_v<U, std::string>, const char*> c_str() {
		return value.c_str();
	}

	operator T() const { return value; }
	void dump() override { notifyOsc(); }

	using OscContainer::operator=;
	OscReadOnlyVariable& operator=(const T& v) {
		set(v);
		return *this;
	}
	OscReadOnlyVariable& operator=(const OscReadOnlyVariable<T>& v) {
		set(v.value);
		return *this;
	}
	template<typename U = T>
	std::enable_if_t<std::is_same_v<U, std::string>, OscReadOnlyVariable&> operator=(const char* v) {
		set(v);
		return *this;
	}
	bool operator==(const OscReadOnlyVariable<T>& other) { return value == other.value; }
	bool operator!=(const OscReadOnlyVariable<T>& other) { return !(*this == other); }

	std::string getAsString() const override {
		if constexpr(std::is_same_v<T, std::string>) {
			return "\"" + getToOsc() + "\"";
		} else {
			return std::to_string(getToOsc());
		}
	}

	void setOscConverters(std::function<T(T)> convertToOsc, std::function<T(T)> convertFromOsc) {
		this->convertToOsc = convertToOsc;
		this->convertFromOsc = convertFromOsc;
	}

	void setChangeCallback(std::function<void(T)> onChange) {
		this->onChange = onChange;
		onChange(this->get());
	}

protected:
	template<class U> friend class OscGenericArray;

	void notifyOsc() {
		OscArgument valueToSend = getToOsc();
		sendMessage(&valueToSend, 1);
	}

	T getToOsc() const {
		if(!convertToOsc)
			return get();
		else
			return convertToOsc(get());
	}
	void setFromOsc(T value) {
		if(!convertFromOsc)
			set(value);
		else
			set(convertFromOsc(value));
	}

private:
	T value{};

	std::function<T(T)> convertToOsc;
	std::function<T(T)> convertFromOsc;
	std::function<void(T)> onChange;
};
