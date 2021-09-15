#include "OscVariable.h"
#include "OscRoot.h"
#include <spdlog/spdlog.h>

EXPLICIT_INSTANCIATE_OSC_VARIABLE(template, OscVariable)

template<typename T>
OscVariable<T>::OscVariable(OscContainer* parent, std::string name, T initialValue, bool fixedSize) noexcept
    : OscReadOnlyVariable<T>(parent, name, initialValue), fixedSize(fixedSize) {
	if(fixedSize)
		return;

	this->getRoot()->addPendingConfigNode(this);

	if constexpr(std::is_same_v<T, bool>) {
		subEndpoint.emplace_back(new OscEndpoint(this, "toggle"))->setCallback([this](auto) {
			SPDLOG_INFO("{}: Toggling", this->getFullAddress());
			this->setFromOsc(!this->getToOsc());
		});
	} else if constexpr(std::is_same_v<T, std::string>) {
		// No toggle/increment/decrement
	} else {
		incrementAmount = (T) 1;
		subEndpoint.emplace_back(new OscEndpoint(this, "increment"))
		    ->setCallback([this](const std::vector<OscArgument>& arguments) {
			    T amount = incrementAmount;

			    if(!arguments.empty()) {
				    OscNode::getArgumentAs<T>(arguments[0], amount);
			    }

			    SPDLOG_INFO("{}: Incrementing by {}", this->getFullAddress(), amount);
			    this->setFromOsc(this->getToOsc() + amount);
		    });
		subEndpoint.emplace_back(new OscEndpoint(this, "decrement"))
		    ->setCallback([this](const std::vector<OscArgument>& arguments) {
			    T amount = incrementAmount;

			    if(!arguments.empty()) {
				    OscNode::getArgumentAs<T>(arguments[0], amount);
			    }

			    SPDLOG_INFO("{}: Decrementing by {}", this->getFullAddress(), amount);
			    this->setFromOsc(this->getToOsc() - amount);
		    });
	}
}

template<typename T> OscVariable<T>& OscVariable<T>::operator=(const OscVariable<T>& v) {
	this->set(v.get());
	return *this;
}

template<typename T> void OscVariable<T>::execute(const std::vector<OscArgument>& arguments) {
	if(fixedSize) {
		SPDLOG_WARN("{}: variable is fixed, ignoring OSC write", this->getFullAddress());
		return;
	}

	if(!arguments.empty()) {
		T v;
		if(this->template getArgumentAs<T>(arguments[0], v)) {
			this->setFromOsc(std::move(v));
		}
	}
}
