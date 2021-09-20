#pragma once

#include <QDialog>

namespace Ui {
class Reverb;
}  // namespace Ui

class Reverb : public QDialog {
	Q_OBJECT

public:
	explicit Reverb(QWidget* parent);
	~Reverb();

	void show();

signals:
	void parameterChanged(size_t channel, float balance);

protected:
	virtual void hideEvent(QHideEvent* event) override;

private:
	Ui::Reverb* ui;
	std::vector<Reverb*> innerReverbs;
	QRect savedGeometry;
};
