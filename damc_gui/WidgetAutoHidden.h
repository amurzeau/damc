#pragma once

#include <QLabel>
#include <QObject>
#include <QWidget>
#include <unordered_map>

class WidgetAutoHidden : public QObject {
public:
	WidgetAutoHidden() noexcept;

	void addContainerWidgets(std::initializer_list<QWidget*> containerWidgets);

protected:
	virtual bool eventFilter(QObject* watched, QEvent* event) override;

	void updateContainerVisibility(QWidget* containerWidget);
	void adjustParentDialog(QWidget* widget);

private:
	std::unordered_map<QObject*, QWidget*> containerWidgetsByChildMap;
	std::unordered_multimap<QWidget*, QWidget*> childByContainerWidgetMap;
	std::unordered_map<QObject*, QLabel*> labelWidgetsByWidgetMap;
	bool widgetsShown;
};
