#include "OscWidgetArray.h"
#include <QWidget>
#include <algorithm>
#include <set>
#include <type_traits>

OscWidgetArray::OscWidgetArray(OscContainer* parent, std::string name) noexcept
    : OscContainer(parent, name), layout(nullptr), keysEndpoint(this, "keys") {}

void OscWidgetArray::setWidget(QWidget* parentWidget,
                               QBoxLayout* layout,
                               OscWidgetFactoryFunction widgetFactoryFunction) {
	this->parentWidget = parentWidget;
	this->layout = layout;
	this->widgetFactoryFunction = widgetFactoryFunction;
}

void OscWidgetArray::addWidget(const std::string& key) {
	qDebug("Adding %s to %s", key.c_str(), getFullAddress().c_str());

	auto it = childWidgets.find(key);
	if(it != childWidgets.end())
		return;

	QWidget* newWidget = widgetFactoryFunction(parentWidget, this, key);
	layout->addWidget(newWidget);
	keysOrder.push_back(key);
	childWidgets.insert(std::make_pair(key, newWidget));
}

void OscWidgetArray::removeWidget(const std::string& key) {
	auto it = childWidgets.find(key);
	if(it == childWidgets.end())
		return;

	qDebug("Removing %s from %s", key.c_str(), getFullAddress().c_str());

	layout->removeWidget(it->second);
	keysOrder.erase(std::remove(keysOrder.begin(), keysOrder.end(), key), keysOrder.end());
	delete it->second;
	childWidgets.erase(it);
}

void OscWidgetArray::swapWidgets(const std::string& sourceKey, const std::string& targetKey, bool insertBefore) {
	auto itSource = childWidgets.find(sourceKey);
	auto itTarget = childWidgets.find(targetKey);

	if(itSource == childWidgets.end() || itTarget == childWidgets.end()) {
		qDebug("Can't find either %s or %s", sourceKey.c_str(), targetKey.c_str());
		return;
	}

	int originalPosition = layout->indexOf(itSource->second);
	layout->removeWidget(itSource->second);
	keysOrder.erase(std::remove(keysOrder.begin(), keysOrder.end(), itSource->first), keysOrder.end());

	int insertPosition = layout->indexOf(itTarget->second);
	if(insertPosition >= 0) {
		if(!insertBefore)
			insertPosition++;

		layout->insertWidget(insertPosition, itSource->second);
		keysOrder.insert(keysOrder.begin() + insertPosition, itSource->first);

		std::vector<OscArgument> keys;
		for(const auto& key : keysOrder) {
			keys.push_back(key);
		}
		keysEndpoint.sendMessage(&keys[0], keys.size());
	} else {
		layout->insertWidget(originalPosition, itSource->second);
		keysOrder.insert(keysOrder.begin() + originalPosition, itSource->first);
	}
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
	} else if(address == "keys") {
		std::set<std::string> keys;
		std::vector<std::string> keyToRemove;

		printf("%s: recv keys: ", getFullAddress().c_str());
		for(const auto& arg : arguments) {
			std::string key;

			if(!getArgumentAs<std::string>(arg, key))
				return;

			printf("%s ", key.c_str());

			keys.insert(key);
			addWidget(key);
		}
		printf("\n");

		printf("%s: own keys: ", getFullAddress().c_str());
		for(const auto& childWidget : childWidgets) {
			if(keys.count(childWidget.first) == 0) {
				keyToRemove.push_back(childWidget.first);
			} else {
				printf("%s ", childWidget.first.c_str());
			}
		}
		printf("\n");

		for(const auto& key : keyToRemove) {
			removeWidget(key);
		}
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
