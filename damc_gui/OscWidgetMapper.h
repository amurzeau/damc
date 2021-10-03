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

template<class T> struct OscWidgetMapperType {};

template<> struct OscWidgetMapperType<QAbstractSlider> { using type = int32_t; };
template<> struct OscWidgetMapperType<QAbstractButton> { using type = bool; };
template<> struct OscWidgetMapperType<QDoubleSpinBox> { using type = float; };
template<> struct OscWidgetMapperType<QSpinBox> { using type = int32_t; };
template<> struct OscWidgetMapperType<QComboBox> { using type = int32_t; };
template<> struct OscWidgetMapperType<QGroupBox> { using type = bool; };
template<> struct OscWidgetMapperType<QLineEdit> { using type = std::string; };

template<class T, class UnderlyingType = typename OscWidgetMapperType<T>::type>
class OscWidgetMapper : public QObject, public OscContainer {
public:
	OscWidgetMapper(OscContainer* parent, std::string name) noexcept;

	void setWidget(T* widget, bool updateOnChange = true);
	void setScale(double scale);
	UnderlyingType get() const { return value; }

	void execute(const std::vector<OscArgument>& arguments) override;

	void dump() override;
	std::string getAsString() const override { return std::string{}; }

	void addChangeCallback(std::function<void(UnderlyingType)> onChange);

protected:
	void notifyChanged(T* originatorWidget);
	void updateWidget(T* widget);

private:
	std::vector<T*> widgets;
	std::vector<std::function<void(UnderlyingType)>> onChangeCallbacks;
	double scale;
	UnderlyingType value;
};

extern template class OscWidgetMapper<QAbstractSlider>;
extern template class OscWidgetMapper<QAbstractButton>;
extern template class OscWidgetMapper<QDoubleSpinBox>;
extern template class OscWidgetMapper<QSpinBox>;
extern template class OscWidgetMapper<QComboBox>;
extern template class OscWidgetMapper<QComboBox, std::string>;
extern template class OscWidgetMapper<QGroupBox>;
extern template class OscWidgetMapper<QLineEdit>;
