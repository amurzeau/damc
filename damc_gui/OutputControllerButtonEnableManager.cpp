#include "OutputControllerButtonEnableManager.h"

OutputControllerButtonEnableManager::OutputControllerButtonEnableManager() : enableButton(nullptr) {}

OutputControllerButtonEnableManager::~OutputControllerButtonEnableManager() {}

void OutputControllerButtonEnableManager::setEnableButton(QAbstractButton* button) {
	enableButton = button;
}

void OutputControllerButtonEnableManager::addOscEnableVariable(OscWidgetMapper<QAbstractButton>* oscVariable) {
	oscEnableVariables.push_back(oscVariable);
	oscVariable->addChangeCallback([this](bool isEnabled) { updateEnableButtonState(); });
}

void OutputControllerButtonEnableManager::updateEnableButtonState() {
	if(!enableButton)
		return;

	bool isEnabled = false;
	for(OscWidgetMapper<QAbstractButton>* item : oscEnableVariables) {
		if(item->get()) {
			isEnabled = true;
			break;
		}
	}
	enableButton->setChecked(isEnabled);
}