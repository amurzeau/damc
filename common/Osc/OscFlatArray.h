#pragma once

#include "OscContainer.h"
#include "Utils.h"
#include <type_traits>

template<typename T> class OscFlatArray : protected OscContainer {
public:
	OscFlatArray(OscContainer* parent, std::string name) noexcept : OscContainer(parent, name) {}

	template<class U> bool updateData(const U& lambda);
	const std::vector<T>& getData() const;
	bool setData(const std::vector<T>& newData);
	bool setData(std::vector<T>&& newData);

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

template<class T> template<class U> bool OscFlatArray<T>::updateData(const U& lambda) {
	std::vector<T> savedValues = values;
	lambda(values);
	for(const auto& key : values) {
		if(std::count(values.begin(), values.end(), key) != 1) {
			printf("Duplicate data inserted: %s\n", key.c_str());
			abort();
		}
	}
	if(values != savedValues) {
		notifyOsc();
		return true;
	}
	return false;
}

template<typename T> const std::vector<T>& OscFlatArray<T>::getData() const {
	return values;
}

template<typename T> bool OscFlatArray<T>::setData(const std::vector<T>& newData) {
	return updateData([&newData](std::vector<std::string>& data) { data = newData; });
}

template<typename T> bool OscFlatArray<T>::setData(std::vector<T>&& newData) {
	return updateData([newData = std::move(newData)](std::vector<std::string>& data) { data = std::move(newData); });
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
	auto oldData = values;

	bool dataChanged = updateData([this, &arguments](std::vector<std::string>& data) {
		data.clear();
		for(const auto& arg : arguments) {
			T v;
			if(this->template getArgumentAs<T>(arg, v)) {
				data.push_back(v);
			} else {
				printf("Bad argument type: %d\n", (int) arg.index());
			}
		}
	});
	if(dataChanged && onChange)
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
