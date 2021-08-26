#include "OscContainer.h"
#include "Utils.h"

bool OscContainer::osc_node_comparator::operator()(const std::string& x, const std::string& y) const {
	bool isXnumber = Utils::isNumber(x);
	bool isYnumber = Utils::isNumber(y);

	// Always put non number nodes first and order numbers by their numerical value
	if(!isXnumber && isYnumber)
		return true;
	else if(isXnumber && !isYnumber)
		return false;
	else if(isXnumber && isYnumber)
		return atoi(x.c_str()) < atoi(y.c_str());
	else {
		return x < y;
	}
}

OscContainer::OscContainer(OscContainer* parent, std::string name) noexcept
    : OscNode(parent, name), oscDump(this, "dump") {
	oscDump.setCallback([this](auto) { dump(); });
}

OscContainer::~OscContainer() {
	auto childrenToDetach = std::move(children);

	for(auto& child : childrenToDetach) {
		child.second->setOscParent(nullptr);
	}
}

void OscContainer::splitAddress(std::string_view address,
                                std::string_view* childAddress,
                                std::string_view* remainingAddress) {
	size_t nextSlash = address.find('/');
	if(childAddress)
		*childAddress = address.substr(0, nextSlash);

	if(remainingAddress && nextSlash != std::string::npos && nextSlash + 1 < address.size()) {
		*remainingAddress = address.substr(nextSlash + 1);
	}
}

void OscContainer::execute(std::string_view address, const std::vector<OscArgument>& arguments) {
	// printf("Executing %s from %s\n", std::string(address).c_str(), getFullAddress().c_str());

	if(address.empty() || address == "/") {
		printf("Executing address %s\n", getFullAddress().c_str());
		execute(arguments);
	} else {
		std::string_view childAddress;
		std::string_view remainingAddress;
		std::string childAddressStr;

		splitAddress(address, &childAddress, &remainingAddress);

		childAddressStr = std::string(childAddress);

		if(childAddressStr == "*") {
			// Wildcard
			for(auto& child : children) {
				// printf("Child address: %s remaining: %s\n", childAddressStr.c_str(),
				// std::string(remainingAddress).c_str());
				child.second->execute(remainingAddress, arguments);
			}
		} else if(childAddressStr == "**") {
			// Double Wildcard: for each child, pass the remaining + the current address for recursive wildcard

			// Skip duplicate **
			while(remainingAddress.size() >= 3 && remainingAddress.substr(0, 3) == "**/")
				splitAddress(remainingAddress, nullptr, &remainingAddress);

			execute(remainingAddress, arguments);

			for(auto& child : children) {
				// printf("Child address: %s remaining: %s\n", childAddressStr.c_str(),
				// std::string(remainingAddress).c_str());

				child.second->execute(address, arguments);
			}
		} else {
			auto it = children.find(childAddressStr);
			if(it == children.end()) {
				// printf("Address %s not found from %s\n", childAddressStr.c_str(), getFullAddress().c_str());
				return;
			}

			// printf("Child address: %s remaining: %s\n", childAddressStr.c_str(),
			// std::string(remainingAddress).c_str());
			it->second->execute(remainingAddress, arguments);
		}
	}
}

bool OscContainer::visit(const std::function<bool(OscNode*)>* nodeVisitorFunction) {
	if(!OscNode::visit(nodeVisitorFunction))
		return false;

	for(auto& child : children) {
		child.second->visit(nodeVisitorFunction);
	}

	return true;
}

std::string OscContainer::getAsString() const {
	std::string result;
	static size_t depth = 0;

	std::string indent;
	for(size_t i = 0; i < depth; i++)
		indent += "\t";

	if(!children.empty()) {
		result += "{";

		depth += 1;
		size_t processedItems = 0;
		for(const auto& child : children) {
			std::string childData = child.second->getAsString();

			if(childData.empty()) {
				printf("%s has no data\n", child.first.c_str());
				continue;
			}

			if(processedItems > 0) {
				result += ",\n";
			} else {
				result += "\n";
			}
			result += indent + "\t\"" + child.first + "\": " + childData;
			processedItems++;
		}
		depth -= 1;

		if(processedItems == 0)
			result.clear();
		else {
			result += "\n";
			result += indent + "}";
		}
	}

	return result;
}

void OscContainer::addChild(std::string name, OscNode* child) {
	if(children.emplace(name, child).second == false) {
		printf("Error adding child %s to %s, already existing\n", name.c_str(), getFullAddress().c_str());
	}
}

void OscContainer::removeChild(std::string name) {
	children.erase(name);
}
