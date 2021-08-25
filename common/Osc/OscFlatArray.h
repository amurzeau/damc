#pragma once

#include "OscContainer.h"

template<typename T> class OscFlatArray : protected OscContainer {
public:
	OscFlatArray(OscContainer* parent, std::string name) noexcept : OscContainer(parent, name) {}

	template<class U> void updateData(const U& lambda);
	const std::vector<T>& getData() const;
	void setData(const std::vector<T>& newData);
	void setData(std::vector<T>&& newData);
	std::vector<T> clearToModify();

	static void erase(std::vector<T>& v, T value);

	void dump() override { notifyOsc(); }

	std::string getAsString() const override;

	void execute(const std::vector<OscArgument>& arguments) override;

	void setChangeCallback(std::function<void(const std::vector<T>&, const std::vector<T>&)> onChange);

protected:
	void notifyOsc();

private:
	std::vector<T> values;
	std::function<void(const std::vector<T>&, const std::vector<T>&)> onChange;
};

template<class T> template<class U> void OscFlatArray<T>::updateData(const U& lambda) {
	lambda(values);
	notifyOsc();
}

template<typename T> const std::vector<T>& OscFlatArray<T>::getData() const {
	return values;
}

template<typename T> void OscFlatArray<T>::setData(const std::vector<T>& newData) {
	values = newData;
	notifyOsc();
}

template<typename T> void OscFlatArray<T>::setData(std::vector<T>&& newData) {
	values = std::move(newData);
	notifyOsc();
}

template<typename T> std::vector<T> OscFlatArray<T>::clearToModify() {
	std::vector<T> ret = std::move(values);
	values.clear();
	return ret;
}

template<typename T> void OscFlatArray<T>::erase(std::vector<T>& v, T value) {
	for(auto it = v.begin(); it != v.end();) {
		if(*it == value) {
			it = v.erase(it);
		} else {
			++it;
		}
	}
}

template<typename T> std::string OscFlatArray<T>::getAsString() const {
	std::string result = "[";

	for(const auto& item : values) {
		if constexpr(std::is_same_v<T, std::string>) {
			result += " \"" + item + "\",";
		} else {
			result += " " + std::to_string(item) + ",";
		}
	}

	if(result.back() == ',')
		result.pop_back();

	return result + " ]";
}

template<typename T> void OscFlatArray<T>::execute(const std::vector<OscArgument>& arguments) {
	auto oldData = std::move(values);
	values.clear();
	for(const auto& arg : arguments) {
		T v;
		if(this->template getArgumentAs<T>(arg, v)) {
			values.push_back(v);
		} else {
			printf("Bad argument type: %d\n", (int) arg.index());
		}
	}
	onChange(oldData, values);
}

template<typename T>
void OscFlatArray<T>::setChangeCallback(std::function<void(const std::vector<T>&, const std::vector<T>&)> onChange) {
	this->onChange = onChange;
}

template<typename T> void OscFlatArray<T>::notifyOsc() {
	std::vector<OscArgument> valueToSend;
	valueToSend.reserve(values.size());
	for(auto& v : values) {
		valueToSend.push_back(v);
	}
	sendMessage(&valueToSend[0], valueToSend.size());
}
