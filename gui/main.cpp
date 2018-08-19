#include "MainWindow.h"
#include "MessageInterface.h"
#include <QApplication>

Q_DECLARE_METATYPE(notification_message_t)

int main(int argc, char* argv[]) {
	QApplication a(argc, argv);

	qRegisterMetaType<notification_message_t>("notification_message_t");

	MainWindow w;
	w.show();

	return a.exec();
}
