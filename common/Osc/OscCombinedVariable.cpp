#include "OscCombinedVariable.h"

OscCombinedVariable::OscCombinedVariable() noexcept : isAllVariableSet(false) {}

void OscCombinedVariable::addVariable(std::unique_ptr<OscSourceVariable> variable,
                                      bool callOnChangeWhenReady,
                                      std::function<bool()> checkFunction) {
	// Need to add it before calling checkVariable else it will declare it as ready
	variables.push_back(VariableData{});
	VariableData& variableData = variables.back();

	variableData.variable = std::move(variable);
	variableData.checker = checkFunction;
	variableData.callOnChangeWhenReady = callOnChangeWhenReady;

	if(checkFunction) {
		variableData.variable->addCheckCallback([checkFunction]() -> bool { return checkFunction(); });
	}
	variableData.variable->addChangeCallback([this]() { checkVariables(); });
}

void OscCombinedVariable::setCallback(std::function<void()> callback) {
	this->callback = callback;
	if(isAllVariableSet && callback)
		callback();
}

bool OscCombinedVariable::isVariablesReady() {
	for(const auto& variable : variables) {
		if(variable.variable->isDefault())
			return false;
		if(variable.checker && !variable.checker())
			return false;
	}
	return true;
}

void OscCombinedVariable::checkVariables() {
	if(isAllVariableSet == false && isVariablesReady()) {
		isAllVariableSet = true;
		if(callback) {
			callback();
		}

		for(const auto& variable : variables) {
			if(variable.callOnChangeWhenReady) {
				variable.variable->callChangeCallback();
			}
		}
	}
}
