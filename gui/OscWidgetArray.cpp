#include "OscWidgetArray.h"
#include <QWidget>
#include <type_traits>

OscWidgetArray::OscWidgetArray(OscContainer* parent, std::string name) noexcept
    : OscContainer(parent, name), widget(nullptr) {}

void OscWidgetArray::setWidget(QWidget* parentWidget,
                               QBoxLayout* widget,
                               OscWidgetFactoryFunction widgetFactoryFunction) {
	this->parentWidget = parentWidget;
	this->widget = widget;
	this->widgetFactoryFunction = widgetFactoryFunction;
}

void OscWidgetArray::addWidget(const std::string& key) {
	qDebug("Adding %s to %s", key.c_str(), getFullAddress().c_str());

	QWidget* newWidget = widgetFactoryFunction(parentWidget, this, key);
	widget->addWidget(newWidget);
	childWidgets.insert(std::make_pair(key, newWidget));
}

void OscWidgetArray::removeWidget(const std::string& key) {
	auto it = childWidgets.find(key);
	if(it == childWidgets.end())
		return;

	qDebug("Removing %s from %s", key.c_str(), getFullAddress().c_str());

	childWidgets.erase(it);
	widget->removeWidget(it->second);
	delete it->second;
}

bool OscWidgetArray::isIndexAddress(std::string_view s) {
	for(char c : s) {
		if(c < '0' || c > '9')
			return false;
	}
	// empty string not considered as a number
	return !s.empty();
}

void OscWidgetArray::execute(std::string_view address, const std::vector<OscArgument>& arguments) {
	if(address == "add") {
		if(arguments.size() != 1)
			return;

		std::string newKey;

		if(!getArgumentAs<std::string>(arguments[0], newKey))
			return;

		addWidget(newKey);
	} else if(address == "remove") {
		if(arguments.size() != 1)
			return;

		std::string removedKey;

		if(!getArgumentAs<std::string>(arguments[0], removedKey))
			return;

		removeWidget(removedKey);
	} else {
		std::string_view childAddress;

		splitAddress(address, &childAddress, nullptr);

		if(!childAddress.empty() && isIndexAddress(childAddress)) {
			std::string key = std::string(childAddress);

			if(childWidgets.count(key) == 0) {
				addWidget(key);
			}
		}

		OscContainer::execute(address, arguments);
	}
}
