#include "OscContainer.h"
#include "Utils.h"
#include <OscRoot.h>
#include <spdlog/spdlog.h>

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
	if(address.empty() || address == "/") {
		SPDLOG_TRACE("Executing address {}", getFullAddress());
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
				child.second->execute(remainingAddress, arguments);
			}
		} else if(childAddressStr == "**") {
			// Double Wildcard: for each child, pass the remaining + the current address for recursive wildcard

			// Skip duplicate **
			while(remainingAddress.size() >= 3 && remainingAddress.substr(0, 3) == "**/")
				splitAddress(remainingAddress, nullptr, &remainingAddress);

			execute(remainingAddress, arguments);

			for(auto& child : children) {
				child.second->execute(address, arguments);
			}
		} else {
			auto it = children.find(childAddressStr);
			if(it == children.end()) {
				SPDLOG_WARN("Address {} not found from {}", childAddressStr, getFullAddress());
				return;
			}

			it->second->execute(remainingAddress, arguments);
		}
	}
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
			if(!child.second->isPersisted())
				continue;
			std::string childData = child.second->getAsString();

			if(childData.empty()) {
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
		SPDLOG_ERROR("Error adding child {} to {}, already existing\n", name, getFullAddress());
	}
}

void OscContainer::removeChild(OscNode* node, std::string name) {
	OscRoot* root = getRoot();
	if(root)
		root->nodeRemoved(node);
	children.erase(name);
}
