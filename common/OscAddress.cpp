#include "OscAddress.h"
#include "OscRoot.h"
#include <math.h>

float ConverterLogScale::fromOsc(float value) {
	return powf(10, value / 20.0f);
}

float ConverterLogScale::toOsc(float value) {
	return 20.0 * log10(value);
}

void OscEndpoint::setCallback(std::function<void(const std::vector<OscArgument>&)> onExecute) {
	this->onExecute = std::move(onExecute);
}

void OscEndpoint::execute(const std::vector<OscArgument>& arguments) {
	if(onExecute)
		onExecute(arguments);
}

OscNode::OscNode(OscContainer* parent, std::string name) noexcept : name(name), parent(nullptr) {
	setOscParent(parent);
}

OscNode::~OscNode() {
	if(parent)
		parent->removeChild(name);
}

void OscNode::setOscParent(OscContainer* parent) {
	if(parent == this->parent)
		return;

	if(this->parent) {
		this->parent->removeChild(name);
		this->parent = nullptr;
	}

	if(parent) {
		parent->addChild(name, this);
		fullAddress = parent->getFullAddress() + "/" + name;
	} else if(!name.empty()) {
		fullAddress = "/" + name;
	} else {
		fullAddress = "";
	}
	this->parent = parent;
}

bool OscNode::visit(const std::function<bool(OscNode*)>* nodeVisitorFunction) {
	if(nodeVisitorFunction) {
		printf("Executing address %s\n", getFullAddress().c_str());
		if(!(*nodeVisitorFunction)(this))
			return false;
	}

	return true;
}

void OscNode::sendMessage(const OscArgument* arguments, size_t number) {
	sendMessage(getFullAddress(), arguments, number);
}

void OscNode::sendMessage(const std::string& address, const OscArgument* arguments, size_t number) {
	if(parent)
		parent->sendMessage(address, arguments, number);
}

bool OscNode::notifyOscAtInit() {
	if(parent)
		return parent->notifyOscAtInit();

	return false;
}

void OscNode::execute(std::string_view address, const std::vector<OscArgument>& arguments) {
	if(address.empty() || address == "/") {
		execute(arguments);
	}
}

bool OscContainer::osc_node_comparator::operator()(const std::string& x, const std::string& y) const {
	// Always put size node first
	if(x == SIZE_NODE && y != SIZE_NODE)
		return true;
	else if(y == SIZE_NODE)
		return false;
	else
		return x < y;
}

OscContainer::OscContainer(OscContainer* parent, std::string name) noexcept
    : OscNode(parent, name), oscDump(this, "dump") {
	oscDump.setCallback([this](auto) {
		auto value = this->getValue();
		if(value) {
			this->sendMessage(&value.value(), 1);
		}
	});
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

std::string OscContainer::getAsString() {
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
