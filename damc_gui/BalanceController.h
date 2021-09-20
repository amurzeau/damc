#pragma once

#include "OscWidgetArray.h"
#include <QDialog>

namespace Ui {
class BalanceController;
class Balance;
}  // namespace Ui

class Balance;

class BalanceController : public QDialog {
	Q_OBJECT

public:
	explicit BalanceController(QWidget* parent, OscContainer* oscParent);
	~BalanceController();

public slots:
	void show();

protected:
	virtual void hideEvent(QHideEvent* event) override;

private:
	Ui::BalanceController* ui;
	std::vector<Balance*> balances;
	OscWidgetArray oscBalances;
	QRect savedGeometry;
};
