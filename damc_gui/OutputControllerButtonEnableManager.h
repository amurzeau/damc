#pragma once

#include "OscWidgetMapper.h"
#include <QAbstractButton>
#include <vector>

class OutputControllerButtonEnableManager {
public:
	explicit OutputControllerButtonEnableManager();
	~OutputControllerButtonEnableManager();

	void setEnableButton(QAbstractButton* button);
	void addOscEnableVariable(OscWidgetMapper<QAbstractButton>* oscVariable);

protected:
	void updateEnableButtonState();

private:
	std::vector<OscWidgetMapper<QAbstractButton>*> oscEnableVariables;
	QAbstractButton* enableButton;
};
