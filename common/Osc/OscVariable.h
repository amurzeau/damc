#pragma once

#include "OscReadOnlyVariable.h"

template<typename T> class OscVariable : public OscReadOnlyVariable<T> {
public:
	OscVariable(OscContainer* parent, std::string name, T initialValue = {}, bool fixedSize = false) noexcept;

	using OscReadOnlyVariable<T>::operator=;
	OscVariable& operator=(const OscVariable<T>& v);

	void execute(const std::vector<OscArgument>& arguments) override;

	void setIncrementAmount(T amount) { incrementAmount = amount; }

private:
	T incrementAmount;
	std::vector<std::unique_ptr<OscEndpoint>> subEndpoint;
	bool fixedSize;
};

template<typename T>
OscVariable<T>::OscVariable(OscContainer* parent, std::string name, T initialValue, bool fixedSize) noexcept
    : OscReadOnlyVariable<T>(parent, name, initialValue), fixedSize(fixedSize) {
	if(fixedSize)
		return;

	if constexpr(std::is_same_v<T, bool>) {
		subEndpoint.emplace_back(new OscEndpoint(this, "toggle"))->setCallback([this](auto) {
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

			    this->setFromOsc(this->getToOsc() + amount);
		    });
		subEndpoint.emplace_back(new OscEndpoint(this, "decrement"))
		    ->setCallback([this](const std::vector<OscArgument>& arguments) {
			    T amount = incrementAmount;

			    if(!arguments.empty()) {
				    OscNode::getArgumentAs<T>(arguments[0], amount);
			    }

			    this->setFromOsc(this->getToOsc() - amount);
		    });
	}
}

template<typename T> OscVariable<T>& OscVariable<T>::operator=(const OscVariable<T>& v) {
	this->set(v.get());
	return *this;
}

template<typename T> void OscVariable<T>::execute(const std::vector<OscArgument>& arguments) {
	if(fixedSize)
		return;

	if(!arguments.empty()) {
		T v;
		if(this->template getArgumentAs<T>(arguments[0], v)) {
			this->setFromOsc(std::move(v));
		} else {
			printf("Bad argument 0 type: %d\n", (int) arguments[0].index());
		}
	}
}
