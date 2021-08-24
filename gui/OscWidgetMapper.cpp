#include "OscWidgetMapper.h"
#include <type_traits>

template<class T>
OscWidgetMapper<T>::OscWidgetMapper(OscContainer* parent, std::string name) noexcept
    : OscContainer(parent, name), scale(1.0) {}

template<class T> void OscWidgetMapper<T>::setWidget(T* widget, bool updateOnChange) {
	this->widgets.push_back(widget);

	if(updateOnChange) {
		if constexpr(std::is_base_of_v<QAbstractButton, T> || std::is_base_of_v<QGroupBox, T>) {
			connect(widget, &T::toggled, [this](bool value) {
				OscArgument valueToSend = value;
				sendMessage(&valueToSend, 1);
			});
		} else if constexpr(std::is_base_of_v<QDoubleSpinBox, T>) {
			connect(widget, qOverload<double>(&T::valueChanged), [this](double value) {
				OscArgument valueToSend = (float) value / scale;
				sendMessage(&valueToSend, 1);
			});
		} else if constexpr(std::is_base_of_v<QComboBox, T>) {
			connect(widget, qOverload<int>(&T::currentIndexChanged), [this](int value) {
				OscArgument valueToSend = (int32_t) value;
				sendMessage(&valueToSend, 1);
			});
		} else {
			connect(widget, qOverload<int>(&T::valueChanged), [this](int value) {
				OscArgument valueToSend = (int32_t)(value / scale);
				sendMessage(&valueToSend, 1);
			});
		}
	}
}

template<class T> void OscWidgetMapper<T>::setScale(float scale) {
	this->scale = scale;
}

template<class T> void OscWidgetMapper<T>::execute(const std::vector<OscArgument>& arguments) {
	float value;

	if(widgets.empty())
		return;

	if(arguments.empty())
		return;

	if(!getArgumentAs<float>(arguments[0], value))
		return;

	for(T* widget : widgets) {
		widget->blockSignals(true);

		if constexpr(std::is_base_of_v<QAbstractButton, T> || std::is_base_of_v<QGroupBox, T>) {
			T* w = widget;
			w->setChecked(value);
		} else if constexpr(std::is_base_of_v<QComboBox, T>) {
			QComboBox* w = widgets.front();
			w->setCurrentIndex((int) value);
		} else {
			widget->setValue(value * scale);
		}

		widget->blockSignals(false);
	}

	if(onChange)
		onChange(value);
}

template<class T> std::optional<OscArgument> OscWidgetMapper<T>::getValue() const {
	if(widgets.empty())
		return {};

	if constexpr(std::is_base_of_v<QAbstractButton, T> || std::is_base_of_v<QGroupBox, T>) {
		T* w = widgets.front();
		return OscArgument{w->isChecked()};
	} else if constexpr(std::is_base_of_v<QDoubleSpinBox, T>) {
		QDoubleSpinBox* w = widgets.front();
		return OscArgument{(float) w->value()};
	} else if constexpr(std::is_base_of_v<QComboBox, T>) {
		QComboBox* w = widgets.front();
		return OscArgument{(int32_t) w->currentIndex()};
	} else {
		return OscArgument{(int32_t) widgets.front()->value()};
	}
}

template<class T> void OscWidgetMapper<T>::setChangeCallback(std::function<void(float)> onChange) {
	this->onChange = onChange;
}

template class OscWidgetMapper<QAbstractSlider>;
template class OscWidgetMapper<QAbstractButton>;
template class OscWidgetMapper<QSpinBox>;
template class OscWidgetMapper<QDoubleSpinBox>;
template class OscWidgetMapper<QComboBox>;
template class OscWidgetMapper<QGroupBox>;
