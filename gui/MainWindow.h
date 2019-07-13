#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "NewOutputInstanceDialog.h"
#include "OutputController.h"
#include "WavePlayInterface.h"
#include "WavePlayOutputInterface.h"
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

protected slots:
	void onMessage(const QJsonObject& message);
	void onAddInstance();
	void onRemoveInstance();

protected:
	void clearOutputs();

private:
	Ui::MainWindow* ui;

	WavePlayInterface wavePlayInterface;
	WavePlayOutputInterface mainControlInterface;
	std::unordered_map<int, OutputController*> outputs;
	int numEq;

	NewOutputInstanceDialog addDialog;
	bool openAddDialogRequested;
};

#endif  // MAINWINDOW_H
