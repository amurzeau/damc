#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "NewOutputInstanceDialog.h"
#include "OscWidgetArray.h"
#include "WavePlayInterface.h"
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

	void moveOutputInstance(int sourceInstance, int targetInstance, bool insertBefore);
	bool getShowDisabledOutputInstances();

protected slots:
	void onAddInstance();
	void onRemoveInstance();

signals:
	void showDisabledChanged();

private:
	Ui::MainWindow* ui;

	OscRoot oscRoot;
	WavePlayInterface wavePlayInterface;
	OscWidgetArray outputInterfaces;

	NewOutputInstanceDialog addDialog;
};

#endif  // MAINWINDOW_H
