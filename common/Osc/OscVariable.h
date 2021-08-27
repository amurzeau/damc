#pragma once

#include "OscReadOnlyVariable.h"
#include <memory>

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

EXPLICIT_INSTANCIATE_OSC_VARIABLE(extern template, OscVariable)
