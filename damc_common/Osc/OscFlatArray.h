#pragma once

#include "OscContainer.h"

template<typename T> class OscFlatArray : protected OscContainer {
public:
	OscFlatArray(OscContainer* parent, std::string name) noexcept;

	template<class U> bool updateData(const U& lambda, bool fromOsc = false);
	const std::vector<T>& getData() const;
	bool setData(const std::vector<T>& newData);
	bool setData(std::vector<T>&& newData);

	void dump() override { notifyOsc(); }

	std::string getAsString() const override;

	void execute(const std::vector<OscArgument>& arguments) override;

	void addCheckCallback(std::function<bool(const std::vector<T>&)> checkCallback);
	void addChangeCallback(std::function<void(const std::vector<T>&, const std::vector<T>&)> onChange);

	bool callCheckCallbacks(const std::vector<T>& v);

protected:
	void notifyOsc();
	bool checkData(const std::vector<T>& savedValues, bool fromOsc);

private:
	std::vector<T> values;
	std::vector<std::function<void(const std::vector<T>&, const std::vector<T>&)>> onChangeCallbacks;
	std::vector<std::function<bool(const std::vector<T>&)>> checkCallbacks;
};

EXPLICIT_INSTANCIATE_OSC_VARIABLE(extern template, OscFlatArray);

template<class T> template<class U> bool OscFlatArray<T>::updateData(const U& lambda, bool fromOsc) {
	std::vector<T> savedValues = values;
	lambda(values);
	return checkData(savedValues, fromOsc);
}
