#ifndef BALANCE_H
#define BALANCE_H

#include "OscWidgetMapper.h"
#include <QWidget>

namespace Ui {
class Balance;
}

class Balance : public QWidget, public OscWidgetMapper<QDoubleSpinBox> {
public:
	explicit Balance(QWidget* parent, OscContainer* oscParent, const std::string& name);
	~Balance();

private:
	Ui::Balance* ui;
};

#endif  // BALANCE_H
