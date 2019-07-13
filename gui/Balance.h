#ifndef BALANCE_H
#define BALANCE_H

#include <QWidget>

namespace Ui {
class Balance;
}

class Balance : public QWidget {
	Q_OBJECT

public:
	explicit Balance(QWidget* parent, size_t index);
	~Balance();

	void setBalance(float volume);

signals:
	void balanceChanged(size_t index, float balance);

protected slots:
	void onBalanceChanged();

private:
	Ui::Balance* ui;
	size_t index;
};

#endif  // BALANCE_H
