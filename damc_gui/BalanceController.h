#pragma once

#include "OscWidgetArray.h"
#include <QDialog>

namespace Ui {
class BalanceController;
class Balance;
}  // namespace Ui

class Balance;
class OutputController;

class BalanceController : public QDialog {
	Q_OBJECT

public:
	explicit BalanceController(OutputController* parent, OscContainer* oscParent);
	~BalanceController();

	bool getEnabled();

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
