#include "OscVariable.h"
#include "OscRoot.h"
#include <spdlog/spdlog.h>

EXPLICIT_INSTANCIATE_OSC_VARIABLE(template, OscVariable)

template<typename T>
OscVariable<T>::OscVariable(OscContainer* parent, std::string name, T initialValue, bool persistValue) noexcept
    : OscReadOnlyVariable<T>(parent, name, initialValue) {
	if(persistValue) {
		this->getRoot()->addPendingConfigNode(this);

		this->addChangeCallback([this](T) { this->getRoot()->notifyValueChanged(); });
	}

	if constexpr(std::is_same_v<T, bool>) {
		subEndpoint.emplace_back(new OscEndpoint(this, "toggle"))->setCallback([this](auto) {
			SPDLOG_DEBUG("{}: Toggling", this->getFullAddress());
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

			    SPDLOG_DEBUG("{}: Incrementing by {}", this->getFullAddress(), amount);
			    this->setFromOsc(this->getToOsc() + amount);
		    });
		subEndpoint.emplace_back(new OscEndpoint(this, "decrement"))
		    ->setCallback([this](const std::vector<OscArgument>& arguments) {
			    T amount = incrementAmount;

			    if(!arguments.empty()) {
				    OscNode::getArgumentAs<T>(arguments[0], amount);
			    }

			    SPDLOG_DEBUG("{}: Decrementing by {}", this->getFullAddress(), amount);
			    this->setFromOsc(this->getToOsc() - amount);
		    });
	}
}

template<typename T> OscVariable<T>& OscVariable<T>::operator=(const OscVariable<T>& v) {
	this->set(v.get());
	return *this;
}

template<typename T> void OscVariable<T>::execute(const std::vector<OscArgument>& arguments) {
	if(!arguments.empty()) {
		T v;
		if(this->template getArgumentAs<T>(arguments[0], v)) {
			this->setFromOsc(std::move(v));
		}
	}
}

template<typename T> std::string OscVariable<T>::getAsString() const {
	if(this->isDefault())
		return {};

	if constexpr(std::is_same_v<T, std::string>) {
		return "\"" + this->getToOsc() + "\"";
	} else {
		return std::to_string(this->getToOsc());
	}
}
