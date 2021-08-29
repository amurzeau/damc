#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "NewOutputInstanceDialog.h"
#include "OscWidgetArray.h"
#include "WavePlayInterface.h"
#include <Osc/OscFlatArray.h>
#include <OscRoot.h>
#include <QWidget>

namespace Ui {
class MainWindow;
}

class MainWindow : public QWidget {
	Q_OBJECT

public:
	explicit MainWindow(QWidget* parent = 0);
	~MainWindow();

	void removeInstance(int key);
	void moveOutputInstance(int sourceInstance, int targetInstance, bool insertBefore);
	bool getShowDisabledOutputInstances();

	const std::vector<std::string>& getTypeList() { return oscTypeArray.getData(); }
	const std::vector<std::string>& getDeviceList() { return oscPortaudioDeviceArray.getData(); }
	const std::vector<std::string>& getDeviceWasapiList() { return oscWasapiDeviceArray.getData(); }

protected slots:
	void onAddInstance();
	void onRemoveInstance();

signals:
	void showDisabledChanged();
	void typeListChanged();
	void deviceListChanged();

private:
	Ui::MainWindow* ui;

	OscRoot oscRoot;
	WavePlayInterface wavePlayInterface;
	OscWidgetArray outputInterfaces;

	OscFlatArray<std::string> oscTypeArray;
	OscFlatArray<std::string> oscPortaudioDeviceArray;
	OscFlatArray<std::string> oscWasapiDeviceArray;
	NewOutputInstanceDialog addDialog;
};

#endif  // MAINWINDOW_H
