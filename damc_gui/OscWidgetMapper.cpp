#include "OscWidgetMapper.h"
#include "QtUtils.h"  // IWYU pragma: keep: needed for qOverload on older Qt versions (< 5.7)
#include <charconv>
#include <spdlog/spdlog.h>
#include <sstream>
#include <stdexcept>
#include <type_traits>

template<class T, class UnderlyingType>
OscWidgetMapper<T, UnderlyingType>::OscWidgetMapper(OscContainer* parent,
                                                    std::string name,
                                                    UnderlyingType defaultValue) noexcept
    : OscContainer(parent, name), value(defaultValue), defaultValue(true) {}

template<class T, class UnderlyingType>
void OscWidgetMapper<T, UnderlyingType>::setWidget(T* widget, bool updateOnChange) {
	this->widgets.push_back(widget);

	widget->setVisible(false);

	widget->setProperty(OSC_MAPPED_PROPERTY, true);

	updateWidget(widget);

	if(updateOnChange) {
		if constexpr(std::is_base_of_v<QAbstractButton, T> || std::is_base_of_v<QGroupBox, T>) {
			if(widget->isCheckable()) {
				connect(widget, &T::toggled, [this, widget](bool value) {
					this->value = value;
					notifyChanged(widget);
				});
			} else {
				connect(widget, &T::clicked, [this, widget](bool value) {
					this->value = 1;
					notifyChanged(widget);
					this->value = 0;
					notifyChanged(widget);
				});
			}
		} else if constexpr(std::is_base_of_v<QDoubleSpinBox, T>) {
			connect(widget, qOverload<double>(&T::valueChanged), [this, widget](double value) {
				this->value = (float) mapValue(value, false);
				notifyChanged(widget);
			});
		} else if constexpr(std::is_base_of_v<QComboBox, T>) {
			if constexpr(std::is_same_v<int32_t, UnderlyingType>) {
				connect(widget, qOverload<int>(&T::currentIndexChanged), [this, widget](int value) {
					this->value = (int32_t) mapValue(value, false);
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
				this->value = (int32_t) mapValue(value, false);
				notifyChanged(widget);
			});
		}
	}
}

template<class T, class UnderlyingType> void OscWidgetMapper<T, UnderlyingType>::setScale(double scale) {
	if constexpr(std::is_same_v<UnderlyingType, int32_t> || std::is_same_v<UnderlyingType, float>) {
		setValueMappingCallback([scale](UnderlyingType value, bool toWidget) {
			if(toWidget)
				return value * scale;
			else
				return value / scale;
		});
	}
}

template<class T, class UnderlyingType>
void OscWidgetMapper<T, UnderlyingType>::setValueMappingCallback(
    std::function<UnderlyingType(UnderlyingType, bool)> callback) {
	mappingCallback = callback;
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

	for(T* widget : widgets) {
		if(widget->isHidden()) {
			widget->setVisible(true);
		}
	}

	this->defaultValue = false;

	if(this->value == value)
		return;

	this->value = value;

	for(T* widget : widgets) {
		updateWidget(widget);
	}

	for(auto& onChange : onChangeCallbacks)
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

template<class T, class UnderlyingType> std::string OscWidgetMapper<T, UnderlyingType>::getAsString() const {
	if(this->isDefault())
		return {};

	if constexpr(std::is_same_v<UnderlyingType, std::string>) {
		return "\"" + this->value + "\"";
	} else if constexpr(std::is_same_v<UnderlyingType, bool>) {
		return value ? "true" : "false";
	} else {
		std::ostringstream oss;
		oss.imbue(std::locale::classic());
		oss << this->value;
		return oss.str();
	}
}

template<class T, class UnderlyingType>
void OscWidgetMapper<T, UnderlyingType>::addChangeCallback(std::function<void(UnderlyingType)> onChange) {
	this->onChangeCallbacks.push_back(onChange);
	onChange(value);
}

template<class T, class UnderlyingType> bool OscWidgetMapper<T, UnderlyingType>::isPersisted() {
	bool persist = true;

	if constexpr(std::is_base_of_v<QAbstractButton, T> || std::is_base_of_v<QAbstractSlider, T> ||
	             std::is_base_of_v<QComboBox, T> || std::is_base_of_v<QGroupBox, T>) {
		return persist;
	} else {
		for(auto& widget : widgets) {
			if(widget->isReadOnly()) {
				persist = false;
			}
		}
	}

	return persist;
}

template<class T, class UnderlyingType> void OscWidgetMapper<T, UnderlyingType>::notifyChanged(T* originatorWidget) {
	for(T* widget : widgets) {
		if(widget != originatorWidget)
			updateWidget(widget);
	}

	for(auto& onChange : onChangeCallbacks)
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
			w->setCurrentIndex(mapValue(value, true));
		} else {
			w->setCurrentText(QString::fromStdString(value));
		}
	} else if constexpr(std::is_base_of_v<QLineEdit, T>) {
		QLineEdit* w = widgets.front();
		w->setText(QString::fromStdString(value));
	} else {
		widget->setValue(mapValue(value, true));
	}

	widget->blockSignals(false);
}

template<class T, class UnderlyingType>
UnderlyingType OscWidgetMapper<T, UnderlyingType>::mapValue(UnderlyingType value, bool toWidget) {
	if(mappingCallback)
		return mappingCallback(value, toWidget);
	return value;
}

template class OscWidgetMapper<QAbstractSlider>;
template class OscWidgetMapper<QAbstractButton>;
template class OscWidgetMapper<QSpinBox>;
template class OscWidgetMapper<QDoubleSpinBox>;
template class OscWidgetMapper<QComboBox>;
template class OscWidgetMapper<QComboBox, std::string>;
template class OscWidgetMapper<QGroupBox>;
template class OscWidgetMapper<QLineEdit>;
