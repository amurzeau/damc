#pragma once

#include <OscAddress.h>
#include <QObject>

#include <QAbstractButton>
#include <QAbstractSlider>
#include <QDoubleSpinBox>
#include <QSpinBox>

template<class T> class OscWidgetMapper : public QObject, public OscNode {
public:
	OscWidgetMapper(OscContainer* parent, std::string name) noexcept;

	void setWidget(T* widget);

	void execute(const std::vector<OscArgument>& arguments) override;

	std::string getAsString() override { return std::string{}; }

private:
	T* widget;
};

extern template class OscWidgetMapper<QAbstractSlider>;
extern template class OscWidgetMapper<QAbstractButton>;
extern template class OscWidgetMapper<QDoubleSpinBox>;
extern template class OscWidgetMapper<QSpinBox>;
