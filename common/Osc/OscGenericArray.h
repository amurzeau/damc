#pragma once

#include "OscContainer.h"
#include "OscFlatArray.h"
#include "Utils.h"
#include <set>

template<typename T> class OscGenericArray : protected OscContainer {
public:
	using OscFactoryFunction = std::function<T*(OscContainer*, int)>;

public:
	OscGenericArray(OscContainer* parent, std::string name) noexcept;

	void setFactory(OscFactoryFunction factoryFunction);

	T& operator[](size_t index) { return *value[index]; }
	const T& operator[](size_t index) const { return *value[index]; }

	int getNextKey();
	template<typename... Args> void push_back(Args... args);
	template<typename... Args> void insert(int key, Args... args);
	void erase(int key);
	template<typename... Args> void resize(size_t size, Args... args);

	auto size() const { return value.size(); }
	auto begin() const { return value.begin(); }
	auto begin() { return value.begin(); }
	auto end() const { return value.end(); }
	auto end() { return value.end(); }
	auto& back() { return value.rbegin()->second; }
	const auto& back() const { return value.crbegin()->second; }

	void execute(std::string_view address, const std::vector<OscArgument>& arguments) override;

protected:
	virtual void initializeItem(T*) {}

private:
	OscFlatArray<int> keys;
	std::map<int, std::unique_ptr<T>> value;
	OscFactoryFunction factoryFunction;
	int nextKey;
};

template<typename T>
OscGenericArray<T>::OscGenericArray(OscContainer* parent, std::string name) noexcept
    : OscContainer(parent, name), keys(this, "keys"), nextKey(0) {
	keys.setChangeCallback([this](const std::vector<int>&, const std::vector<int>& newValue) {
		for(const auto& oldKey : value) {
			if(!Utils::vector_find(newValue, oldKey.first)) {
				erase(oldKey.first);
			}
		}
		for(const auto& newKey : newValue) {
			if(value.count(newKey) == 0) {
				T* newValue = factoryFunction(this, newKey);

				initializeItem(newValue);
				value.insert(std::make_pair(newKey, std::unique_ptr<T>{newValue}));
			}
		}
	});
}

template<typename T> void OscGenericArray<T>::setFactory(OscFactoryFunction factoryFunction) {
	this->factoryFunction = factoryFunction;
}

template<typename T> int OscGenericArray<T>::getNextKey() {
	int newKey = nextKey;
	nextKey++;
	return newKey;
}

template<typename T> template<typename... Args> void OscGenericArray<T>::push_back(Args... args) {
	insert(getNextKey(), std::forward<Args>(args)...);
}

template<typename T> template<typename... Args> void OscGenericArray<T>::insert(int newKey, Args... args) {
	if(nextKey <= newKey)
		nextKey = newKey + 1;

	keys.updateData([&newKey](std::vector<int>& keys) { keys.push_back(newKey); });

	T* newValue = factoryFunction(this, newKey);

	initializeItem(newValue);
	value.insert(std::make_pair(newKey, std::unique_ptr<T>(newValue)));
}

template<typename T> void OscGenericArray<T>::erase(int key) {
	for(auto it = value.begin(); it != value.end();) {
		if(it->first == key) {
			value.erase(it);
			break;
		} else {
			++it;
		}
	}

	keys.updateData([&key](std::vector<int>& data) { Utils::vector_erase(data, key); });
}

template<typename T>
void OscGenericArray<T>::execute(std::string_view address, const std::vector<OscArgument>& arguments) {
	if(address == "keys") {
		std::vector<int> newKeys;
		std::set<int> keysToKeep;
		const auto& keysOrder = keys.getData();

		for(const auto& arg : arguments) {
			int key;

			if(!getArgumentAs<int>(arg, key))
				return;

			newKeys.push_back(key);
		}

		for(size_t i = 0; i < newKeys.size(); i++) {
			int key = newKeys[i];

			keysToKeep.insert(key);

			if(std::count(keysOrder.begin(), keysOrder.end(), key) == 0) {
				// The item wasn't existing, add it
				insert(key);
			}
		}

		std::vector<int> keyToRemove;
		for(int key : keysOrder) {
			if(keysToKeep.count(key) == 0) {
				keyToRemove.push_back(key);
			}
		}

		for(const auto& key : keyToRemove) {
			erase(key);
		}

		keys.setData(newKeys);
	} else {
		std::string_view childAddress;

		splitAddress(address, &childAddress, nullptr);

		if(!childAddress.empty() && Utils::isNumber(childAddress)) {
			int key = atoi(std::string(childAddress).c_str());

			if(value.count(key) == 0) {
				insert(key);
			}
		}

		OscContainer::execute(address, arguments);
	}
}

template<typename T> template<typename... Args> void OscGenericArray<T>::resize(size_t newSize, Args... args) {
	if(newSize == value.size())
		return;

	if(newSize < value.size()) {
		while(value.size() > newSize) {
			erase(value.rbegin()->first);
		}
	} else {
		while(value.size() < newSize) {
			push_back(args...);
		}
	}
}
