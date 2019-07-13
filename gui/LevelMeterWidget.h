#ifndef LEVELMETERWIDGET_H
#define LEVELMETERWIDGET_H

#include <QTime>
#include <QTimer>
#include <QWidget>

class LevelMeterWidget : public QWidget {
	Q_OBJECT

public:
	explicit LevelMeterWidget(QWidget* parent = 0);
	~LevelMeterWidget();

	void paintEvent(QPaintEvent* event) override;

public slots:
	void setValue(float peakLevel);

private slots:
	void redrawTimerExpired();

protected:
	float getNormalizedLevel(float levelDb);

private:
	/**
	 * Height of peak level bar.
	 * This is calculated by decaying m_peakLevel depending on the
	 * elapsed time since m_peakLevelChanged, and the value of m_decayRate.
	 */
	float m_decayedPeakLevel;

	/**
	 * High watermark of peak level.
	 * Range 0.0 - 1.0.
	 */
	float m_peakHoldLevel;

	/**
	 * Time at which m_peakHoldLevel was last changed.
	 */
	QTime m_peakHoldLevelChanged;

	QTimer* m_redrawTimer;
};

#endif  // LEVELMETERWIDGET_H
