#pragma once

#include <OscAddress.h>
#include <QObject>

#include <QBoxLayout>
#include <QLayout>
#include <unordered_set>

class QWidget;

class OscWidgetArray : public QObject, public OscContainer {
public:
	using OscWidgetFactoryFunction = std::function<QWidget*(QWidget*, OscContainer*, const std::string&)>;

public:
	OscWidgetArray(OscContainer* parent, std::string name) noexcept;

	void setWidget(QWidget* parentWidget, QBoxLayout* widget, OscWidgetFactoryFunction widgetFactoryFunction);
	std::vector<QWidget*> getWidgets();

	void execute(std::string_view address, const std::vector<OscArgument>& arguments) override;

	std::string getAsString() const override { return std::string{}; }

	void addWidget(const std::string& key);
	void removeWidget(const std::string& key);
	void swapWidgets(const std::string& sourceKey, const std::string& targetKey, bool insertBefore, bool notifyOsc);

private:
	QWidget* parentWidget;
	QBoxLayout* layout;
	std::unordered_map<std::string, QWidget*> childWidgets;
	std::vector<std::string> keysOrder;
	OscWidgetFactoryFunction widgetFactoryFunction;
	OscEndpoint keysEndpoint;
};
