#pragma once

#include <string_view>
#include <vector>

namespace Utils {

bool isNumber(std::string_view s);

template<typename T> bool vector_find(const std::vector<T>& keys, const T& key) {
	for(const auto& existingKey : keys) {
		if(existingKey == key) {
			return true;
		}
	}
	return false;
}

template<typename T> void vector_erase(std::vector<T>& v, T value) {
	for(auto it = v.begin(); it != v.end();) {
		if(*it == value) {
			it = v.erase(it);
		} else {
			++it;
		}
	}
}

}  // namespace Utils
