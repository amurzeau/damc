#include "OscAddress.h"
#include "OscRoot.h"
#include <math.h>

float ConverterLogScale::fromOsc(float value) {
	return powf(10, value / 20.0f);
}

float ConverterLogScale::toOsc(float value) {
	return 20.0 * log10(value);
}

void OscEndpoint::setCallback(std::function<void(std::vector<OscArgument>)> onExecute) {
	this->onExecute = std::move(onExecute);
}

void OscEndpoint::execute(const std::vector<OscArgument>& arguments) {
	if(onExecute)
		onExecute(std::move(arguments));
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

const std::string& OscNode::getFullAddress() {
	return fullAddress;
}

void OscNode::sendMessage(const OscArgument* arguments, size_t number) {
	sendMessage(getFullAddress(), arguments, number);
}

void OscNode::sendMessage(const std::string& address, const OscArgument* arguments, size_t number) {
	if(parent)
		parent->sendMessage(address, arguments, number);
}

void OscNode::execute(std::string_view address, const std::vector<OscArgument>& arguments) {
	execute(arguments);
}

OscContainer::~OscContainer() {
	auto childrenToDetach = std::move(children);

	for(auto& child : childrenToDetach) {
		child.second->setOscParent(nullptr);
	}
}

void OscContainer::execute(std::string_view address, const std::vector<OscArgument>& arguments) {
	printf("Executing %s from %s\n", std::string(address).c_str(), getFullAddress().c_str());

	if(address.empty() || address == "/") {
		printf("Executing address %s\n", getFullAddress().c_str());
		execute(arguments);
	} else {
		size_t nextSlash = address.find('/');
		std::string childAddress = std::string(address.substr(0, nextSlash));

		std::string_view remainingAddress;
		if(nextSlash != std::string::npos && nextSlash + 1 < address.size()) {
			remainingAddress = address.substr(nextSlash + 1);
		}

		if(childAddress != "*") {
			auto it = children.find(childAddress);
			if(it == children.end()) {
				printf("Address %s not found from %s\n", childAddress.c_str(), getFullAddress().c_str());
				return;
			}

			printf("Child address: %s remaining: %s\n", childAddress.c_str(), std::string(remainingAddress).c_str());
			it->second->execute(remainingAddress, arguments);
		} else {
			// Wildcard
			for(auto& child : children) {
				printf(
				    "Child address: %s remaining: %s\n", childAddress.c_str(), std::string(remainingAddress).c_str());
				child.second->execute(remainingAddress, arguments);
			}
		}
	}
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
