#include "WidgetAutoHidden.h"
#include <QDialog>
#include <QEvent>
#include <QGroupBox>
#include <QMetaEnum>
#include <QMetaObject>
#include <QRegularExpression>
#include <spdlog/spdlog.h>

WidgetAutoHidden::WidgetAutoHidden() noexcept : widgetsShown(false) {}

void WidgetAutoHidden::addContainerWidgets(std::initializer_list<QWidget*> containerWidgets) {
	for(QWidget* containerWidget : containerWidgets) {
		SPDLOG_DEBUG("Object {} contains:", containerWidget->objectName().toStdString());

		QList<QLabel*> labelWidgets = containerWidget->findChildren<QLabel*>(Qt::FindDirectChildrenOnly);
		bool isGroupBox = qobject_cast<QGroupBox*>(containerWidget) != nullptr;

		for(QObject* child : containerWidget->children()) {
			QWidget* childAsWidget = qobject_cast<QWidget*>(child);
			if(!childAsWidget)
				continue;

			if(qobject_cast<QLabel*>(child))
				continue;

			SPDLOG_DEBUG(
			    "- {} (class {})", childAsWidget->objectName().toStdString(), childAsWidget->metaObject()->className());

			// Recurse in QGroupBox and QWidget
			if(qobject_cast<QGroupBox*>(child) ||
			   strcmp(child->metaObject()->className(), QWidget::staticMetaObject.className()) == 0) {
				addContainerWidgets({childAsWidget});
			}

			// Add matching label to labels map
			// When a widget is hidden/shown, its matching label is also hidden/shown
			qsizetype longestMatchLength = 0;
			QLabel* longestMatch = nullptr;
			QString childObjectName = child->objectName();
			for(QLabel* label : labelWidgets) {
				QString baseName = label->objectName().replace("Label", "");
				if((longestMatch == nullptr || baseName.size() > longestMatchLength) &&
				   childObjectName.startsWith(baseName)) {
					longestMatch = label;
					longestMatchLength = baseName.size();
				}
			}

			if(longestMatch) {
				SPDLOG_DEBUG("Found matching QLabel {} for {}",
				             longestMatch->objectName().toStdString(),
				             child->objectName().toStdString());
				this->labelWidgetsByWidgetMap.emplace(child, longestMatch);
				longestMatch->setVisible(!childAsWidget->isHidden());
			}

			// Only handle visibility of GroupBoxes
			if(isGroupBox) {
				this->containerWidgetsByChildMap.emplace(child, containerWidget);
				this->childByContainerWidgetMap.emplace(containerWidget, childAsWidget);
			}

			childAsWidget->installEventFilter(this);
		}

		if(isGroupBox) {
			updateContainerVisibility(containerWidget);
		}
		containerWidget->adjustSize();
	}
}

void WidgetAutoHidden::updateContainerVisibility(QWidget* containerWidget) {
	SPDLOG_DEBUG("Updating visibility of {}", containerWidget->objectName().toStdString());

	bool allHidden = true;
	auto childrenRange = childByContainerWidgetMap.equal_range(containerWidget);
	for(auto itChildInContainer = childrenRange.first; itChildInContainer != childrenRange.second;
	    ++itChildInContainer) {
		if(!itChildInContainer->second->isHidden()) {
			allHidden = false;
			SPDLOG_DEBUG("- {} shown", itChildInContainer->second->objectName().toStdString());
			break;
		} else {
			SPDLOG_DEBUG("- {} hidden", itChildInContainer->second->objectName().toStdString());
		}
	}

	if(allHidden) {
		SPDLOG_DEBUG("all widget in container are hidden, hidding container widget {}",
		             containerWidget->objectName().toStdString());
		if(!containerWidget->isHidden()) {
			containerWidget->setVisible(false);
			adjustParentDialog(containerWidget);
		}
	} else {
		SPDLOG_DEBUG("some widgets are shown, showing container widget {}",
		             containerWidget->objectName().toStdString());
		containerWidget->setVisible(true);
	}
}

void WidgetAutoHidden::adjustParentDialog(QWidget* widget) {
	QWidget* parentWidget = widget;

	while(parentWidget && !qobject_cast<QDialog*>(parentWidget))
		parentWidget = qobject_cast<QWidget*>(widget->parent());

	if(parentWidget) {
		parentWidget->adjustSize();
	}
}

bool WidgetAutoHidden::eventFilter(QObject* watched, QEvent* event) {
	/*static int eventEnumIndex = QEvent::staticMetaObject.indexOfEnumerator("Type");

	QString name = QEvent::staticMetaObject.enumerator(eventEnumIndex).valueToKey(event->type());
	SPDLOG_DEBUG("MainWindow: Event {} on {}", name.toStdString(), watched->objectName().toStdString());
*/
	if(event->type() == QEvent::ShowToParent) {
		auto itLabel = labelWidgetsByWidgetMap.find(watched);
		if(itLabel != labelWidgetsByWidgetMap.end()) {
			itLabel->second->setVisible(true);
		}

		auto it = containerWidgetsByChildMap.find(watched);
		if(it != containerWidgetsByChildMap.end()) {
			QWidget* containerWidget = it->second;

			if(!containerWidget->isHidden())
				return false;

			SPDLOG_DEBUG("widget {} shown, showing container widget {}",
			             watched->objectName().toStdString(),
			             containerWidget->objectName().toStdString());

			containerWidget->setVisible(true);
		}
	} else if(event->type() == QEvent::HideToParent) {
		auto itLabel = labelWidgetsByWidgetMap.find(watched);
		if(itLabel != labelWidgetsByWidgetMap.end()) {
			itLabel->second->setVisible(false);
		}

		auto it = containerWidgetsByChildMap.find(watched);
		if(it != containerWidgetsByChildMap.end()) {
			QWidget* containerWidget = it->second;

			if(containerWidget->isHidden())
				return false;

			SPDLOG_DEBUG("widget {} hidden, updating {} state",
			             watched->objectName().toStdString(),
			             containerWidget->objectName().toStdString());

			updateContainerVisibility(containerWidget);
		} else {
			adjustParentDialog(qobject_cast<QWidget*>(watched));
		}
	}

	return false;
}
