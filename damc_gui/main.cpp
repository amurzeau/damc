#include "MainWindow.h"
#include "SerialPortInterface.h"
#include "WavePlayInterface.h"
#include <OscRoot.h>
#include <QApplication>

int main(int argc, char* argv[]) {
	QApplication a(argc, argv);

	OscRoot oscRoot(false);

	std::unique_ptr<OscConnector> oscConnector;

	qsizetype serialArgIndex = a.arguments().indexOf("--serial");
	if(serialArgIndex >= 0 && a.arguments().size() > serialArgIndex + 1) {
		QString portName = a.arguments().at(serialArgIndex + 1);
		oscConnector.reset(new SerialPortInterface(&oscRoot, portName));
	} else {
		oscConnector.reset(new WavePlayInterface(&oscRoot));
	}

	MainWindow w(&oscRoot);
	w.show();

	return a.exec();
}
