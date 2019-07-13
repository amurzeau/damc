#ifndef BALANCECONTROLLER_H
#define BALANCECONTROLLER_H

#include <QDialog>

namespace Ui {
class BalanceController;
class Balance;
}  // namespace Ui

class Balance;

class BalanceController : public QDialog {
	Q_OBJECT

public:
	explicit BalanceController(QWidget* parent);
	~BalanceController();

	void setChannelNumber(int numChannel);
	void setParameters(size_t channel, float balance);

	void show();

signals:
	void parameterChanged(size_t channel, float balance);

protected:
	virtual void hideEvent(QHideEvent* event) override;

private:
	Ui::BalanceController* ui;
	std::vector<Balance*> balances;
	QRect savedGeometry;
};

#endif  // BALANCECONTROLLER_H
