#ifndef DIALOG_H
#define DIALOG_H

#include "WavePlayOutputInterface.h"
#include <QDialog>
#include <QJsonArray>

namespace Ui {
class NewOutputInstanceDialog;
}

class NewOutputInstanceDialog : public QDialog {
	Q_OBJECT

public:
	explicit NewOutputInstanceDialog(WavePlayOutputInterface* interface, QWidget* parent = 0);
	~NewOutputInstanceDialog();

	void setTypeList(QJsonArray stringArray);
	void setDeviceList(QJsonArray stringArray);
	void setWasapiDeviceList(QJsonArray stringArray);

protected slots:
	void onConfirm();
	void onDeviceTypeChanged();

private:
	Ui::NewOutputInstanceDialog* ui;
	WavePlayOutputInterface* interface;

	QJsonArray portaudioDeviceArray;
	QJsonArray wasapiDeviceArray;
};

#endif  // DIALOG_H
