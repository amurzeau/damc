#ifndef COMPRESSORCONTROLLER_H
#define COMPRESSORCONTROLLER_H

#include <QDialog>

namespace Ui {
class CompressorController;
}

class CompressorController : public QDialog {
	Q_OBJECT

public:
	explicit CompressorController(QWidget* parent);
	~CompressorController();

	void setParameters(bool enabled,
	                   float releaseTime,
	                   float attackTime,
	                   float threshold,
	                   float makeUpGain,
	                   float compressionRatio,
	                   float kneeWidth,
	                   bool useMovingMax);

	void show();

signals:
	void parameterChanged(bool enabled,
	                      float releaseTime,
	                      float attackTime,
	                      float threshold,
	                      float makeUpGain,
	                      float compressionRatio,
	                      float kneeWidth,
	                      bool useMovingMax);

protected slots:
	void onParameterChanged();

protected:
	virtual void hideEvent(QHideEvent* event) override;

private:
	Ui::CompressorController* ui;
	QRect savedGeometry;
};

#endif  // COMPRESSORCONTROLLER_H
