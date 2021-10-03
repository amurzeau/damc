#pragma once

#include "OscReadOnlyVariable.h"
#include <functional>
#include <memory>
#include <vector>

class OscSourceVariable {
public:
	virtual bool isDefault() const = 0;
	virtual void addCheckCallback(std::function<bool()> callback) = 0;
	virtual void addChangeCallback(std::function<void()> callback) = 0;
	virtual void callChangeCallback() = 0;
};

template<class T> class OscSourceTemplatedVariable : public OscSourceVariable {
public:
	OscSourceTemplatedVariable(OscReadOnlyVariable<T>* variable) : variable(variable) {}
	bool isDefault() const override { return variable->isDefault(); };
	void addCheckCallback(std::function<bool()> callback) override {
		variable->addCheckCallback([callback](T) -> bool { return callback(); });
	}
	void addChangeCallback(std::function<void()> callback) override {
		variable->addChangeCallback([callback](T) { callback(); });
	}
	void callChangeCallback() override { variable->callChangeCallbacks(variable->get()); }

private:
	OscReadOnlyVariable<T>* variable;
};

class OscCombinedVariable {
public:
	OscCombinedVariable() noexcept;

	template<class T>
	void addVariable(OscReadOnlyVariable<T>* variable,
	                 bool callOnChangeWhenReady = false,
	                 std::function<bool()> checkFunction = std::function<bool()>{});

	void addVariable(std::unique_ptr<OscSourceVariable> variable,
	                 bool callOnChangeWhenReady = false,
	                 std::function<bool()> checkFunction = std::function<bool()>{});

	// Called only when all variables are not default value and every time one of them changes
	void setCallback(std::function<void()> callback);

	bool isVariablesReady();

protected:
	void checkVariables();

private:
	struct VariableData {
		std::unique_ptr<OscSourceVariable> variable;
		std::function<bool()> checker;
		bool callOnChangeWhenReady;
	};

	std::vector<VariableData> variables;
	std::function<void()> callback;
	bool isAllVariableSet;
	bool isCheckingVariables;
};

template<class T>
void OscCombinedVariable::addVariable(OscReadOnlyVariable<T>* variable,
                                      bool callOnChangeWhenReady,
                                      std::function<bool()> checkFunction) {
	addVariable(std::make_unique<OscSourceTemplatedVariable<T>>(variable), callOnChangeWhenReady, checkFunction);
}
