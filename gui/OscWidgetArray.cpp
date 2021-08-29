#include "OscWidgetArray.h"
#include "Utils.h"
#include <QWidget>
#include <algorithm>
#include <set>
#include <type_traits>

OscWidgetArray::OscWidgetArray(OscContainer* parent, std::string name) noexcept
    : OscContainer(parent, name), layout(nullptr), keysEndpoint(this, "keys"), nextKey(0) {}

void OscWidgetArray::setWidget(QWidget* parentWidget,
                               QBoxLayout* layout,
                               OscWidgetFactoryFunction widgetFactoryFunction) {
	this->parentWidget = parentWidget;
	this->layout = layout;
	this->widgetFactoryFunction = widgetFactoryFunction;
}

QWidget* OscWidgetArray::getWidget(int key) {
	auto it = childWidgets.find(key);
	if(it == childWidgets.end())
		return nullptr;

	return it->second;
}

std::vector<QWidget*> OscWidgetArray::getWidgets() {
	std::vector<QWidget*> ret;
	for(const auto& key : keysOrder) {
		auto it = childWidgets.find(key);
		if(it != childWidgets.end()) {
			ret.push_back(it->second);
		}
	}
	return ret;
}

int OscWidgetArray::addItem() {
	int key = nextKey;
	nextKey++;

	addWidget(key);
	notifyOsc();

	return key;
}

void OscWidgetArray::removeItem(int key) {
	removeWidget(key);
	notifyOsc();
}

void OscWidgetArray::addWidget(int key) {
	qDebug("Adding %d to %s", key, getFullAddress().c_str());

	auto it = childWidgets.find(key);
	if(it != childWidgets.end())
		return;

	QWidget* newWidget = widgetFactoryFunction(parentWidget, this, key);
	layout->addWidget(newWidget);
	childWidgets.insert(std::make_pair(key, newWidget));
	keysOrder.push_back(key);
	if(nextKey <= key)
		nextKey = key + 1;
}

void OscWidgetArray::removeWidget(int key) {
	auto it = childWidgets.find(key);
	if(it == childWidgets.end())
		return;

	qDebug("Removing %d from %s", key, getFullAddress().c_str());

	layout->removeWidget(it->second);
	keysOrder.erase(std::remove(keysOrder.begin(), keysOrder.end(), key), keysOrder.end());
	delete it->second;
	childWidgets.erase(it);
}

void OscWidgetArray::swapWidgets(int sourceKey, int targetKey, bool insertBefore, bool notifyOsc) {
	auto itSource = childWidgets.find(sourceKey);
	auto itTarget = childWidgets.find(targetKey);

	if(itSource == childWidgets.end() || itTarget == childWidgets.end()) {
		qDebug("Can't find either %d or %d", sourceKey, targetKey);
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

		if(notifyOsc) {
			this->notifyOsc();
		}
	} else {
		layout->insertWidget(originalPosition, itSource->second);
		keysOrder.insert(keysOrder.begin() + originalPosition, itSource->first);
	}
}

void OscWidgetArray::notifyOsc() {
	std::vector<OscArgument> keys;
	for(const auto& key : keysOrder) {
		keys.push_back(key);
	}
	keysEndpoint.sendMessage(&keys[0], keys.size());
}

void OscWidgetArray::execute(std::string_view address, const std::vector<OscArgument>& arguments) {
	if(address == "keys") {
		std::vector<int> newKeys;
		std::set<int> keysToKeep;

		printf("%s: recv keys: ", getFullAddress().c_str());
		for(const auto& arg : arguments) {
			int key;

			if(!getArgumentAs<int>(arg, key))
				return;

			printf("%d ", key);

			newKeys.push_back(key);
		}
		printf("\n");

		for(size_t i = 0; i < newKeys.size(); i++) {
			int key = newKeys[i];

			keysToKeep.insert(key);

			size_t existingPosition = std::find(keysOrder.begin(), keysOrder.end(), key) - keysOrder.begin();
			if(existingPosition >= keysOrder.size()) {
				// The item wasn't existing, add it and move it
				addWidget(key);
				if(existingPosition >= keysOrder.size()) {
					// keysOrder size should have increased by 1
					printf("New item couldn't be added\n");
					continue;
				}

				// addWidget insert at last position
				existingPosition = keysOrder.size() - 1;
			}
			if(existingPosition != i) {
				// Need to move it
				if(i < keysOrder.size())
					swapWidgets(key, keysOrder[i], true, false);
				else if(i == keysOrder.size())
					swapWidgets(key, keysOrder[keysOrder.size() - 1], false, false);
				else {
					printf("Unhandled index\n");
					abort();
				}
			} else {
				// Position is the same, nothing to do
			}
		}

		std::vector<int> keyToRemove;
		printf("%s: own keys: ", getFullAddress().c_str());
		for(int key : keysOrder) {
			if(keysToKeep.count(key) == 0) {
				keyToRemove.push_back(key);
			} else {
				printf("%d ", key);
			}
		}
		printf("\n");

		for(const auto& key : keyToRemove) {
			removeWidget(key);
		}
	} else if(address == "add" || address == "remove") {
		// nothing to do, already handled by keys updates
	} else {
		std::string_view childAddress;

		splitAddress(address, &childAddress, nullptr);

		if(!childAddress.empty() && Utils::isNumber(childAddress)) {
			int key = atoi(std::string(childAddress).c_str());

			if(childWidgets.count(key) == 0) {
				addWidget(key);
			}
		}

		OscContainer::execute(address, arguments);
	}
}
