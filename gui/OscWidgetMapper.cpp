#include "OscWidgetMapper.h"
#include <type_traits>

template<class T>
OscWidgetMapper<T>::OscWidgetMapper(OscContainer* parent, std::string name) noexcept
    : OscNode(parent, name), widget(nullptr) {}

template<class T> void OscWidgetMapper<T>::setWidget(T* widget) {
	this->widget = widget;

	if constexpr(std::is_base_of_v<QAbstractButton, T>) {
		connect(widget, &QAbstractButton::toggled, [this](bool value) {
			OscArgument valueToSend = value;
			sendMessage(&valueToSend, 1);
		});
	} else if constexpr(std::is_base_of_v<QDoubleSpinBox, T>) {
		connect(widget, qOverload<double>(&T::valueChanged), [this](double value) {
			OscArgument valueToSend = (float) value;
			sendMessage(&valueToSend, 1);
		});
	} else {
		connect(widget, qOverload<int>(&T::valueChanged), [this](int value) {
			OscArgument valueToSend = (int32_t) value;
			sendMessage(&valueToSend, 1);
		});
	}
}

template<class T> void OscWidgetMapper<T>::execute(const std::vector<OscArgument>& arguments) {
	float value;

	if(!widget)
		return;

	if(arguments.empty())
		return;

	if(!getArgumentAs<float>(arguments[0], value))
		return;

	widget->blockSignals(true);

	if constexpr(std::is_base_of_v<QAbstractButton, T>) {
		QAbstractButton* w = widget;
		w->setChecked(value);
	} else {
		widget->setValue(value);
	}

	widget->blockSignals(false);
}

template class OscWidgetMapper<QAbstractSlider>;
template class OscWidgetMapper<QAbstractButton>;
template class OscWidgetMapper<QSpinBox>;
template class OscWidgetMapper<QDoubleSpinBox>;
