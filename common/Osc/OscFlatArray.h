#pragma once

#include "OscContainer.h"

template<typename T> class OscFlatArray : protected OscContainer {
public:
	OscFlatArray(OscContainer* parent, std::string name, bool fixedSize = false) noexcept
	    : OscContainer(parent, name) {}

	template<class U> void updateData(const U& lambda) {
		lambda(values);
		notifyOsc();
	}
	const std::vector<T>& getData() const { return values; }
	void setData(const std::vector<T>& newData) {
		values = newData;
		notifyOsc();
	}
	void setData(std::vector<T>&& newData) {
		values = std::move(newData);
		notifyOsc();
	}
	std::vector<T> clearToModify() {
		std::vector<T> ret = std::move(values);
		values.clear();
		return ret;
	}

	static void erase(std::vector<T>& v, T value) {
		for(auto it = v.begin(); it != v.end();) {
			if(*it == value) {
				it = v.erase(it);
			} else {
				++it;
			}
		}
	}

	void dump() override { notifyOsc(); }

	std::string getAsString() const override {
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

	void execute(const std::vector<OscArgument>& arguments) override {
		auto oldData = std::move(values);
		values.clear();
		for(const auto& arg : arguments) {
			T v;
			if(this->template getArgumentAs<T>(arg, v)) {
				values.push_back(v);
			} else {
				printf("Bad argument type: %d\n", arg.index());
			}
		}
		onChange(oldData, values);
	}

	void setChangeCallback(std::function<void(const std::vector<T>&, const std::vector<T>&)> onChange) {
		this->onChange = onChange;
	}

protected:
	void notifyOsc() {
		std::vector<OscArgument> valueToSend;
		valueToSend.reserve(values.size());
		for(auto& v : values) {
			valueToSend.push_back(v);
		}
		sendMessage(&valueToSend[0], valueToSend.size());
	}

private:
	std::vector<T> values;
	std::function<void(const std::vector<T>&, const std::vector<T>&)> onChange;
};
