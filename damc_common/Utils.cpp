#include "Utils.h"

bool Utils::isNumber(std::string_view s) {
	for(char c : s) {
		if(c < '0' || c > '9')
			return false;
	}
	// empty string not considered as a number
	return !s.empty();
}
