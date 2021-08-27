#pragma once

#include "OscContainer.h"
#include "OscReadOnlyVariable.h"

template<typename T> class OscFlatArray : protected OscContainer {
public:
	OscFlatArray(OscContainer* parent, std::string name) noexcept : OscContainer(parent, name) {}

	template<class U> bool updateData(const U& lambda);
	const std::vector<T>& getData() const;
	bool setData(const std::vector<T>& newData);
	bool setData(std::vector<T>&& newData);

	void dump() override { notifyOsc(); }

	std::string getAsString() const override;

	void execute(const std::vector<OscArgument>& arguments) override;

	void setChangeCallback(std::function<void(const std::vector<T>&)> onChange);

protected:
	void notifyOsc();
	bool checkData(const std::vector<T>& savedValues);

private:
	std::vector<T> values;
	std::vector<std::function<void(const std::vector<T>&)>> onChangeCallbacks;
};

EXPLICIT_INSTANCIATE_OSC_VARIABLE(extern template, OscFlatArray);

template<class T> template<class U> bool OscFlatArray<T>::updateData(const U& lambda) {
	std::vector<T> savedValues = values;
	lambda(values);
	return checkData(savedValues);
}
