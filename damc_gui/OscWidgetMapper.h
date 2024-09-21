#pragma once

#include <Osc/OscContainer.h>
#include <QObject>

#include <QAbstractButton>
#include <QAbstractSlider>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QGroupBox>
#include <QLineEdit>
#include <QSpinBox>
#include <vector>

static constexpr const char* OSC_MAPPED_PROPERTY = "oscMapped";

template<class T> struct OscWidgetMapperType {};

template<> struct OscWidgetMapperType<QAbstractSlider> {
	using type = int32_t;
};
template<> struct OscWidgetMapperType<QAbstractButton> {
	using type = bool;
};
template<> struct OscWidgetMapperType<QDoubleSpinBox> {
	using type = float;
};
template<> struct OscWidgetMapperType<QSpinBox> {
	using type = int32_t;
};
template<> struct OscWidgetMapperType<QComboBox> {
	using type = int32_t;
};
template<> struct OscWidgetMapperType<QGroupBox> {
	using type = bool;
};
template<> struct OscWidgetMapperType<QLineEdit> {
	using type = std::string;
};

template<class T, class UnderlyingType = typename OscWidgetMapperType<T>::type>
class OscWidgetMapper : public QObject, public OscContainer {
public:
	OscWidgetMapper(OscContainer* parent, std::string name, UnderlyingType defaultValue = UnderlyingType{}) noexcept;

	void setWidget(T* widget, bool updateOnChange = true);
	void setScale(double scale);
	void setValueMappingCallback(std::function<UnderlyingType(UnderlyingType, bool)> callback);
	UnderlyingType get() const { return value; }
	bool isDefault() const { return defaultValue; }

	void execute(const std::vector<OscArgument>& arguments) override;

	void dump() override;
	std::string getAsString() const override;

	void addChangeCallback(std::function<void(UnderlyingType)> onChange);
	void addOscEndpointSupportedCallback(std::function<void()> onOscEndpointSupported);

	bool isPersisted() override;

protected:
	void notifyChanged(T* originatorWidget);
	void updateWidget(T* widget);
	UnderlyingType mapValue(UnderlyingType value, bool toWidget);

private:
	std::vector<T*> widgets;
	std::vector<std::function<void(UnderlyingType)>> onChangeCallbacks;
	std::vector<std::function<void()>> onOscEndpointSupportedCallbacks;
	std::function<UnderlyingType(UnderlyingType, bool)> mappingCallback;
	UnderlyingType value;
	bool defaultValue;
};

extern template class OscWidgetMapper<QAbstractSlider>;
extern template class OscWidgetMapper<QAbstractButton>;
extern template class OscWidgetMapper<QDoubleSpinBox>;
extern template class OscWidgetMapper<QSpinBox>;
extern template class OscWidgetMapper<QComboBox>;
extern template class OscWidgetMapper<QComboBox, std::string>;
extern template class OscWidgetMapper<QGroupBox>;
extern template class OscWidgetMapper<QLineEdit>;
