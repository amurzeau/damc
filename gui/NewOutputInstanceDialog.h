#ifndef DIALOG_H
#define DIALOG_H

#include <Osc/OscContainer.h>
#include <Osc/OscFlatArray.h>
#include <QDialog>

namespace Ui {
class NewOutputInstanceDialog;
}

class NewOutputInstanceDialog : public QDialog {
	Q_OBJECT

public:
	explicit NewOutputInstanceDialog(OscContainer* oscParent, QWidget* parent = 0);
	~NewOutputInstanceDialog();

protected slots:
	void onConfirm();
	void onDeviceTypeChanged();

private:
	Ui::NewOutputInstanceDialog* ui;

	OscFlatArray<std::string> oscTypeArray;
	OscFlatArray<std::string> oscPortaudioDeviceArray;
	OscFlatArray<std::string> oscWasapiDeviceArray;
};

#endif  // DIALOG_H
