#include "OscNode.h"
#include "OscContainer.h"
#include "OscRoot.h"

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
	getRoot()->sendMessage(getFullAddress(), arguments, number);
}

void OscNode::execute(std::string_view address, const std::vector<OscArgument>& arguments) {
	if(address.empty() || address == "/") {
		execute(arguments);
	}
}

OscRoot* OscNode::getRoot() {
	return parent->getRoot();
}
