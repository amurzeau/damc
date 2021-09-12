#pragma once

#include "OscContainer.h"
#include "OscReadOnlyVariable.h"

template<typename T> class OscDynamicVariable : public OscContainer {
public:
	using underlying_type = T;

	OscDynamicVariable(OscContainer* parent, std::string name);
	OscDynamicVariable(const OscDynamicVariable&) = delete;

	std::vector<T> get();
	void dump() override { notifyOsc(); }

	std::string getAsString() const override;

	void setReadCallback(std::function<std::vector<T>()> onReadCallback);

protected:
	void notifyOsc();

private:
	std::function<std::vector<T>()> onReadCallback;
};

EXPLICIT_INSTANCIATE_OSC_VARIABLE(extern template, OscDynamicVariable)
