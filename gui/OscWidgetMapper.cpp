#include "OscWidgetMapper.h"
#include <type_traits>

template<class T, class UnderlyingType>
OscWidgetMapper<T, UnderlyingType>::OscWidgetMapper(OscContainer* parent, std::string name) noexcept
    : OscContainer(parent, name), scale(1.0) {}

template<class T, class UnderlyingType>
void OscWidgetMapper<T, UnderlyingType>::setWidget(T* widget, bool updateOnChange) {
	this->widgets.push_back(widget);

	updateWidget(widget);

	if(updateOnChange) {
		if constexpr(std::is_base_of_v<QAbstractButton, T> || std::is_base_of_v<QGroupBox, T>) {
			connect(widget, &T::toggled, [this, widget](bool value) {
				this->value = value;
				notifyChanged(widget);
			});
		} else if constexpr(std::is_base_of_v<QDoubleSpinBox, T>) {
			connect(widget, qOverload<double>(&T::valueChanged), [this, widget](double value) {
				this->value = (float) (value / scale);
				notifyChanged(widget);
			});
		} else if constexpr(std::is_base_of_v<QComboBox, T>) {
			if constexpr(std::is_same_v<int32_t, UnderlyingType>) {
				connect(widget, qOverload<int>(&T::currentIndexChanged), [this, widget](int value) {
					this->value = (int32_t) value;
					notifyChanged(widget);
				});
			} else {
				connect(widget, qOverload<int>(&T::currentIndexChanged), [this, widget](int) {
					std::string newValue = widget->currentText().toStdString();
					if(!newValue.empty()) {
						this->value = newValue;
						notifyChanged(widget);
					}
				});
			}
		} else if constexpr(std::is_base_of_v<QLineEdit, T>) {
			connect(widget, &QLineEdit::editingFinished, [this, widget]() {
				this->value = widget->text().toStdString();
				notifyChanged(widget);
			});
		} else {
			connect(widget, qOverload<int>(&T::valueChanged), [this, widget](int value) {
				this->value = (int32_t)(value / scale);
				notifyChanged(widget);
			});
		}
	}
}

template<class T, class UnderlyingType> void OscWidgetMapper<T, UnderlyingType>::setScale(double scale) {
	this->scale = scale;
}

template<class T, class UnderlyingType>
void OscWidgetMapper<T, UnderlyingType>::execute(const std::vector<OscArgument>& arguments) {
	UnderlyingType value;

	if(widgets.empty())
		return;

	if(arguments.empty())
		return;

	if(!getArgumentAs<UnderlyingType>(arguments[0], value))
		return;

	if(this->value == value)
		return;

	this->value = value;

	for(T* widget : widgets) {
		updateWidget(widget);
	}

	if(onChange)
		onChange(value);
}

template<class T, class UnderlyingType> void OscWidgetMapper<T, UnderlyingType>::dump() {
	if(widgets.empty())
		return;

	OscArgument arg;

	if constexpr(std::is_base_of_v<QAbstractButton, T> || std::is_base_of_v<QGroupBox, T>) {
		T* w = widgets.front();
		arg = OscArgument{w->isChecked()};
	} else if constexpr(std::is_base_of_v<QDoubleSpinBox, T>) {
		QDoubleSpinBox* w = widgets.front();
		arg = OscArgument{(float) w->value()};
	} else if constexpr(std::is_base_of_v<QComboBox, T>) {
		QComboBox* w = widgets.front();
		arg = OscArgument{(int32_t) w->currentIndex()};
	} else if constexpr(std::is_base_of_v<QLineEdit, T>) {
		QLineEdit* w = widgets.front();
		arg = OscArgument{w->text().toStdString()};
	} else {
		arg = OscArgument{(int32_t) widgets.front()->value()};
	}

	sendMessage(&arg, 1);
}

template<class T, class UnderlyingType>
void OscWidgetMapper<T, UnderlyingType>::setChangeCallback(std::function<void(UnderlyingType)> onChange) {
	this->onChange = onChange;
	onChange(value);
}

template<class T, class UnderlyingType> void OscWidgetMapper<T, UnderlyingType>::notifyChanged(T* originatorWidget) {
	for(T* widget : widgets) {
		if(widget != originatorWidget)
			updateWidget(widget);
	}

	if(onChange)
		onChange(value);

	OscArgument valueToSend = value;
	sendMessage(&valueToSend, 1);
}

template<class T, class UnderlyingType> void OscWidgetMapper<T, UnderlyingType>::updateWidget(T* widget) {
	widget->blockSignals(true);

	if constexpr(std::is_base_of_v<QAbstractButton, T> || std::is_base_of_v<QGroupBox, T>) {
		T* w = widget;
		w->setChecked(value);
	} else if constexpr(std::is_base_of_v<QComboBox, T>) {
		QComboBox* w = widgets.front();
		if constexpr(std::is_same_v<int32_t, UnderlyingType>) {
			w->setCurrentIndex(value);
		} else {
			w->setCurrentText(QString::fromStdString(value));
		}
	} else if constexpr(std::is_base_of_v<QLineEdit, T>) {
		QLineEdit* w = widgets.front();
		w->setText(QString::fromStdString(value));
	} else {
		widget->setValue(value * scale);
	}

	widget->blockSignals(false);
}

template class OscWidgetMapper<QAbstractSlider>;
template class OscWidgetMapper<QAbstractButton>;
template class OscWidgetMapper<QSpinBox>;
template class OscWidgetMapper<QDoubleSpinBox>;
template class OscWidgetMapper<QComboBox>;
template class OscWidgetMapper<QComboBox, std::string>;
template class OscWidgetMapper<QGroupBox>;
template class OscWidgetMapper<QLineEdit>;
