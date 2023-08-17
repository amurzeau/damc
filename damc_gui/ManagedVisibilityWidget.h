#pragma once

#include "WidgetAutoHidden.h"

template<class T> class ManagedVisibilityWidget : public T {
public:
	using T::T;
	void manageWidgetsVisiblity() { widgetAutoHidden.addContainerWidgets({this}); }

private:
	WidgetAutoHidden widgetAutoHidden;
};
