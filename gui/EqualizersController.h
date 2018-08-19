#ifndef EQUALIZERSCONTROLLER_H
#define EQUALIZERSCONTROLLER_H

#include <QDialog>

namespace Ui {
class EqualizersController;
}

class Equalizer;

class EqualizersController : public QDialog {
	Q_OBJECT

public:
	explicit EqualizersController(QWidget* parent, int numEq);
	~EqualizersController();

	void connectEqualizers(QObject* obj, const char* slot);
	void setEqualizerParameters(int index, int type, double f0, double q, double gain);

	void show();

protected:
	virtual void hideEvent(QHideEvent* event) override;

private:
	Ui::EqualizersController* ui;
	std::vector<Equalizer*> equalizers;
	QRect savedGeometry;
};

#endif  // EQUALIZERSCONTROLLER_H
