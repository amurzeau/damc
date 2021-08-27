#pragma once

#include <Osc/OscContainer.h>
#include <Osc/OscEndpoint.h>
#include <QObject>

#include <QBoxLayout>
#include <QLayout>
#include <unordered_set>

class QWidget;

class OscWidgetArray : public QObject, public OscContainer {
public:
	using OscWidgetFactoryFunction = std::function<QWidget*(QWidget*, OscContainer*, int)>;

public:
	OscWidgetArray(OscContainer* parent, std::string name) noexcept;

	void setWidget(QWidget* parentWidget, QBoxLayout* widget, OscWidgetFactoryFunction widgetFactoryFunction);
	std::vector<QWidget*> getWidgets();

	void execute(std::string_view address, const std::vector<OscArgument>& arguments) override;

	std::string getAsString() const override { return std::string{}; }

	void addWidget(int key);
	void removeWidget(int key);
	void swapWidgets(int sourceKey, int targetKey, bool insertBefore, bool notifyOsc);

private:
	QWidget* parentWidget;
	QBoxLayout* layout;
	std::unordered_map<int, QWidget*> childWidgets;
	std::vector<int> keysOrder;
	OscWidgetFactoryFunction widgetFactoryFunction;
	OscEndpoint keysEndpoint;
};
