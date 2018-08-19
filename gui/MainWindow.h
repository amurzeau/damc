#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "OutputController.h"
#include "WavePlayInterface.h"
#include "WavePlayOutputInterface.h"
#include <QWidget>
#include <vector>

namespace Ui {
class MainWindow;
}

class MainWindow : public QWidget {
	Q_OBJECT

public:
	explicit MainWindow(QWidget* parent = 0);
	~MainWindow();

protected slots:
	void onMessage(notification_message_t message);

private:
	Ui::MainWindow* ui;

	WavePlayInterface wavePlayInterface;
	WavePlayOutputInterface mainControlInterface;
	std::vector<OutputController*> outputs;
};

#endif  // MAINWINDOW_H
