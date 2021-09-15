#include "OscNode.h"
#include "OscContainer.h"
#include "OscRoot.h"
#include <spdlog/spdlog.h>

template bool OscNode::getArgumentAs<bool>(const OscArgument& argument, bool& v);
template bool OscNode::getArgumentAs<int32_t>(const OscArgument& argument, int32_t& v);
template bool OscNode::getArgumentAs<float>(const OscArgument& argument, float& v);
template bool OscNode::getArgumentAs<std::string>(const OscArgument& argument, std::string& v);

OscNode::OscNode(OscContainer* parent, std::string name) noexcept : name(name), parent(nullptr) {
	setOscParent(parent);
}

OscNode::~OscNode() {
	if(parent)
		parent->removeChild(name);
}

template<typename T> bool OscNode::getArgumentAs(const OscArgument& argument, T& v) {
	bool ret = false;
	std::visit(
	    [&v, &ret](auto&& arg) -> void {
		    using U = std::decay_t<decltype(arg)>;
		    if constexpr(std::is_same_v<U, T>) {
			    v = arg;
			    ret = true;
		    } else if constexpr(!std::is_same_v<U, std::string> && !std::is_same_v<T, std::string>) {
			    v = (T) arg;
			    ret = true;
		    }
	    },
	    argument);

	if(!ret) {
		SPDLOG_ERROR("{}: Bad argument type: {} is not a {}",
		             this->getFullAddress(),
		             OscRoot::getArgumentVectorAsString(&argument, 1),
		             osc_type_name<T>::name);
	}

	return ret;
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
		SPDLOG_DEBUG("Executing address {}", getFullAddress());
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
