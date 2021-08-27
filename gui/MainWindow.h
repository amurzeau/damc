#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "NewOutputInstanceDialog.h"
#include "OscWidgetArray.h"
#include "OutputController.h"
#include "WavePlayInterface.h"
#include <OscRoot.h>
#include <QJsonObject>
#include <QWidget>
#include <unordered_map>

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
	void onMessage(const QJsonObject& message);
	void onAddInstance();
	void onRemoveInstance();

signals:
	void showDisabledChanged();

protected:
	void clearOutputs();

private:
	Ui::MainWindow* ui;

	OscRoot oscRoot;
	WavePlayInterface wavePlayInterface;
	OscWidgetArray outputInterfaces;
	std::unordered_map<int, OutputController*> outputs;
	int numEq;

	NewOutputInstanceDialog addDialog;
	bool openAddDialogRequested;
};

#endif  // MAINWINDOW_H
