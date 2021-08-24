#pragma once

#include <OscAddress.h>
#include <QObject>

#include <QAbstractButton>
#include <QAbstractSlider>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QGroupBox>
#include <QSpinBox>
#include <vector>

template<class T> class OscWidgetMapper : public QObject, public OscContainer {
public:
	OscWidgetMapper(OscContainer* parent, std::string name) noexcept;

	void setWidget(T* widget, bool updateOnChange = true);
	void setScale(float scale);

	void execute(const std::vector<OscArgument>& arguments) override;

	std::optional<OscArgument> getValue() const override;
	std::string getAsString() override { return std::string{}; }

	void setChangeCallback(std::function<void(float)> onChange);

private:
	std::vector<T*> widgets;
	std::function<void(float)> onChange;
	float scale;
};

extern template class OscWidgetMapper<QAbstractSlider>;
extern template class OscWidgetMapper<QAbstractButton>;
extern template class OscWidgetMapper<QDoubleSpinBox>;
extern template class OscWidgetMapper<QSpinBox>;
extern template class OscWidgetMapper<QComboBox>;
extern template class OscWidgetMapper<QGroupBox>;
