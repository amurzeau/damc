#pragma once

#include "OscContainer.h"
#include "OscFlatArray.h"
#include "Utils.h"
#include <algorithm>
#include <map>
#include <memory>
#include <set>
#include <spdlog/spdlog.h>

template<typename T> class OscGenericArray : protected OscContainer {
public:
	using OscFactoryFunction = std::function<T*(OscContainer*, int32_t)>;

public:
	OscGenericArray(OscContainer* parent, std::string name) noexcept;

	void setFactory(OscFactoryFunction factoryFunction);

	// T& operator[](size_t index) { return *value.at(index); }
	// const T& operator[](size_t index) const { return *value.at(index); }

	T& at(size_t index) { return *value.at(index); }
	const T& at(size_t index) const { return *value.at(index); }
	bool contains(size_t index) const;

	int32_t getNextKey();
	void push_back();
	void insert(int32_t key);
	void erase(int32_t key);
	void resize(size_t size);

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
	void updateNextKeyToMaxKey();

	void insertValue(int32_t key);
	void eraseValue(int32_t key);

private:
	OscFlatArray<int32_t> keys;
	std::map<int32_t, std::unique_ptr<T>> value;
	OscFactoryFunction factoryFunction;
	int32_t nextKey;
};

template<typename T>
OscGenericArray<T>::OscGenericArray(OscContainer* parent, std::string name) noexcept
    : OscContainer(parent, name), keys(this, "keys"), nextKey(0) {
	keys.addChangeCallback([this](const std::vector<int32_t>& oldKeys, const std::vector<int32_t>& newKeys) {
		std::vector<int32_t> keysToKeep;
		bool mustUpdateKeys = false;

		for(size_t i = 0; i < newKeys.size(); i++) {
			int32_t key = newKeys[i];

			if(std::count(keysToKeep.begin(), keysToKeep.end(), key) > 0) {
				SPDLOG_ERROR("{}: Duplicate key {}", this->getFullAddress(), key);
				mustUpdateKeys = true;
				continue;
			}

			keysToKeep.push_back(key);

			if(!Utils::vector_find(oldKeys, key)) {
				// The item wasn't existing, add it
				insertValue(key);
			}
		}

		std::vector<int32_t> keyToRemove;
		for(int32_t key : oldKeys) {
			if(std::count(keysToKeep.begin(), keysToKeep.end(), key) == 0) {
				eraseValue(key);
			}
		}

		if(mustUpdateKeys)
			keys.setData(keysToKeep);
	});
}

template<typename T> void OscGenericArray<T>::setFactory(OscFactoryFunction factoryFunction) {
	this->factoryFunction = factoryFunction;
}

template<typename T> bool OscGenericArray<T>::contains(size_t index) const {
	return value.count(index) > 0;
}

template<typename T> int32_t OscGenericArray<T>::getNextKey() {
	int32_t newKey = nextKey;
	nextKey++;
	return newKey;
}

template<typename T> void OscGenericArray<T>::push_back() {
	insert(getNextKey());
}

template<typename T> void OscGenericArray<T>::insert(int32_t newKey) {
	if(nextKey <= newKey)
		nextKey = newKey + 1;

	keys.updateData([&newKey](std::vector<int32_t>& keys) { keys.push_back(newKey); });
}

template<typename T> void OscGenericArray<T>::erase(int32_t key) {
	keys.updateData([&key](std::vector<int32_t>& data) { Utils::vector_erase(data, key); });
}

template<typename T> void OscGenericArray<T>::resize(size_t newSize) {
	if(newSize == value.size())
		return;

	if(newSize < value.size()) {
		while(value.size() > newSize) {
			erase(value.rbegin()->first);
		}
	} else {
		while(value.size() < newSize) {
			push_back();
		}
	}
}

template<typename T>
void OscGenericArray<T>::execute(std::string_view address, const std::vector<OscArgument>& arguments) {
	std::string_view childAddress;

	splitAddress(address, &childAddress, nullptr);

	if(!childAddress.empty() && Utils::isNumber(childAddress)) {
		int32_t key = atoi(std::string(childAddress).c_str());

		if(value.count(key) == 0) {
			SPDLOG_DEBUG("{}: detect new key {} by direct access", this->getFullAddress(), key);
			insert(key);
		}
	}

	OscContainer::execute(address, arguments);
}

template<typename T> void OscGenericArray<T>::updateNextKeyToMaxKey() {
	int32_t maxKey = 0;
	for(auto it = value.begin(); it != value.end(); ++it) {
		if(it->first + 1 > maxKey || it == value.begin())
			maxKey = it->first + 1;
	}

	nextKey = maxKey;
}

template<typename T> void OscGenericArray<T>::insertValue(int32_t key) {
	SPDLOG_DEBUG("{}: new item {}", this->getFullAddress(), key);

	T* newValue = factoryFunction(this, key);

	initializeItem(newValue);
	value.insert(std::make_pair(key, std::unique_ptr<T>(newValue)));
}

template<typename T> void OscGenericArray<T>::eraseValue(int32_t key) {
	SPDLOG_DEBUG("{}: removing item {}", this->getFullAddress(), key);

	for(auto it = value.begin(); it != value.end();) {
		if(it->first == key) {
			value.erase(it);
			break;
		} else {
			++it;
		}
	}

	updateNextKeyToMaxKey();
}
