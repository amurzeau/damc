#pragma once

#include "OscContainer.h"
#include "OscFlatArray.h"

template<typename T> class OscGenericArray : protected OscContainer {
public:
	OscGenericArray(OscContainer* parent, std::string name, bool fixedSize = false) noexcept
	    : OscContainer(parent, name),
	      oscAddEndpoint(this, "add"),
	      oscRemoveEndpoint(this, "remove"),
	      keys(this, "keys"),
	      nextKey(0) {
		keys.setChangeCallback([this](const std::vector<std::string>&, const std::vector<std::string>& newValue) {
			for(const auto& oldKey : getChildren()) {
				if(!containsKey(newValue, oldKey.first)) {
					erase(oldKey.first);
				}
			}
			for(const auto& newKey : newValue) {
				if(getChildren().count(newKey) == 0) {
					T* newValue = new T(this, newKey);

					initializeItem(newValue);
					value.emplace_back(newValue);
				}
			}
		});
	}

	T& operator[](size_t index) { return *value[index]; }
	const T& operator[](size_t index) const { return *value[index]; }

	static bool containsKey(const std::vector<std::string>& keys, const std::string& key) {
		for(const auto& existingKey : keys) {
			if(existingKey == key) {
				return true;
			}
		}
		return false;
	}

	template<typename... Args> void push_back(Args... args) {
		std::string newKey = std::to_string(nextKey);
		nextKey++;

		OscArgument argument = newKey;
		oscAddEndpoint.sendMessage(&argument, 1);

		keys.updateData([&newKey](std::vector<std::string>& keys) { keys.push_back(newKey); });

		T* newValue = new T(this, newKey, args...);

		initializeItem(newValue);
		value.emplace_back(newValue);
	}

	void pop_back() {
		std::string removedKey = value.back()->getName();
		OscArgument argument = removedKey;
		oscRemoveEndpoint.sendMessage(&argument, 1);

		value.pop_back();

		keys.updateData([this, &removedKey](std::vector<std::string>& data) { keys.erase(data, removedKey); });
	}

	size_t erase(std::string key) {
		const auto& children = this->getChildren();

		auto it = children.find(key);
		if(it == children.end())
			return 0;

		OscArgument argument = key;
		oscRemoveEndpoint.sendMessage(&argument, 1);

		for(auto itVector = value.begin(); itVector != value.end();) {
			OscNode* node = it->second;
			OscNode* checkedNode = itVector->get();
			if(checkedNode == node) {
				value.erase(itVector);
				break;
			} else {
				++itVector;
			}
		}

		keys.updateData([this, &key](std::vector<std::string>& data) { keys.erase(data, key); });
	}

	template<typename... Args> void resize(size_t newSize, Args... args) {
		if(newSize == value.size())
			return;

		if(newSize < value.size()) {
			while(value.size() > newSize) {
				pop_back();
			}
		} else {
			while(value.size() < newSize) {
				push_back(args...);
			}
		}
	}

	auto size() const { return value.size(); }
	auto begin() const { return value.begin(); }
	auto begin() { return value.begin(); }
	auto end() const { return value.end(); }
	auto end() { return value.end(); }
	const auto& back() const { return *value.back(); }
	auto& back() { return *value.back(); }

	std::string getAsString() const override {
		std::string result = "[";

		for(const auto& item : value) {
			result += " " + item->getAsString() + ",";
		}

		if(result.back() == ',')
			result.pop_back();

		return result + " ]";
	}

protected:
	virtual void initializeItem(T*) {}

	OscEndpoint oscAddEndpoint;
	OscEndpoint oscRemoveEndpoint;

private:
	OscFlatArray<std::string> keys;
	std::vector<std::unique_ptr<T>> value;
	size_t nextKey;
};
