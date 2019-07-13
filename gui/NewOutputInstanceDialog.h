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

protected slots:
	void onConfirm();

private:
	Ui::NewOutputInstanceDialog* ui;
	WavePlayOutputInterface* interface;
};

#endif  // DIALOG_H
