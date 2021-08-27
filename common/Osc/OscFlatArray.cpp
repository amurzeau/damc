#include "OscFlatArray.h"
#include <type_traits>

EXPLICIT_INSTANCIATE_OSC_VARIABLE(template, OscFlatArray);

template<typename T> bool OscFlatArray<T>::checkData(const std::vector<T>& savedValues) {
	for(auto key : values) {
		if(std::count(values.begin(), values.end(), key) != 1) {
			printf("Duplicate data inserted\n");
			abort();
		}
	}
	if(values != savedValues) {
		notifyOsc();
		notifyValueChanged();
		return true;
	}
	return false;
}

template<typename T> const std::vector<T>& OscFlatArray<T>::getData() const {
	return values;
}

template<typename T> bool OscFlatArray<T>::setData(const std::vector<T>& newData) {
	return updateData([&newData](std::vector<T>& data) { data = newData; });
}

template<typename T> bool OscFlatArray<T>::setData(std::vector<T>&& newData) {
	return updateData([newData = std::move(newData)](std::vector<T>& data) { data = std::move(newData); });
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
	bool dataChanged = updateData([this, &arguments](std::vector<T>& data) {
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
	if(dataChanged) {
		for(auto& callback : onChangeCallbacks) {
			callback(values);
		}
	}
}

template<typename T> void OscFlatArray<T>::setChangeCallback(std::function<void(const std::vector<T>&)> onChange) {
	this->onChangeCallbacks.push_back(onChange);
}

template<typename T> void OscFlatArray<T>::notifyOsc() {
	std::vector<OscArgument> valueToSend;
	valueToSend.reserve(values.size());
	for(const auto& v : values) {
		valueToSend.push_back(v);
	}
	sendMessage(&valueToSend[0], valueToSend.size());
}
