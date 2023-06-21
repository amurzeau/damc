#pragma once

#include "OscWidgetArray.h"
#include <Osc/OscFlatArray.h>
#include <OscRoot.h>
#include <QWidget>

namespace Ui {
class MainWindow;
}

class GlobalConfigDialog;

class MainWindow : public QWidget {
	Q_OBJECT

public:
	explicit MainWindow(OscRoot* oscRoot, QWidget* parent = 0);
	~MainWindow();

	void removeInstance(int key);
	void moveOutputInstance(int sourceInstance, int targetInstance, bool insertBefore);
	bool getShowDisabledOutputInstances();

	void updateDeviceList();
	const std::vector<std::string>& getTypeList() { return oscTypeArray.getData(); }
	const std::vector<std::string>& getDeviceList() { return oscPortaudioDeviceArray.getData(); }
	const std::vector<std::string>& getDeviceWasapiList() { return oscWasapiDeviceArray.getData(); }

protected slots:
	void onShowGlobalConfig();
	void onAddInstance();
	void onRemoveInstance();

signals:
	void showDisabledChanged();
	void typeListChanged();
	void deviceListChanged();

private:
	Ui::MainWindow* ui;

	OscRoot* oscRoot;
	OscWidgetArray outputInterfaces;

	GlobalConfigDialog* globalConfigDialog;

	OscFlatArray<std::string> oscTypeArray;
	OscFlatArray<std::string> oscPortaudioDeviceArray;
	OscFlatArray<std::string> oscWasapiDeviceArray;
};
