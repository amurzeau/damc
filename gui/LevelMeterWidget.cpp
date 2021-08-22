#include "LevelMeterWidget.h"
#include <math.h>

#include <QDebug>
#include <QPainter>
#include <QTimer>

// Constants
static const int RedrawInterval = 100;          // ms
static const int PeakHoldLevelDuration = 2000;  // ms

static constexpr float MAX_VALUE = 6;
static constexpr float MIN_VALUE = -80;

LevelMeterWidget::LevelMeterWidget(QWidget* parent)
    : QWidget(parent),
      m_decayedPeakLevel(MIN_VALUE),
      m_peakHoldLevel(MIN_VALUE),
      m_redrawTimer(new QTimer(this)),
      backgroundBrush(palette().brush(QPalette::Normal, QPalette::Base)),
      levelBrush(palette().brush(QPalette::Normal, QPalette::ButtonText)),
      peakBrush(palette().brush(QPalette::Normal, QPalette::Highlight)),
      overLevelBrush(Qt::red) {
	setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
	setMinimumWidth(2);

	setAttribute(Qt::WA_OpaquePaintEvent);

	connect(m_redrawTimer, &QTimer::timeout, this, &LevelMeterWidget::redrawTimerExpired);
	m_redrawTimer->start(RedrawInterval);
}

LevelMeterWidget::~LevelMeterWidget() {}

void LevelMeterWidget::setValue(float peakLevel) {
	if(m_decayedPeakLevel != peakLevel) {
		m_decayedPeakLevel = peakLevel;

		if(peakLevel > m_peakHoldLevel) {
			m_peakHoldLevel = peakLevel;
			m_peakHoldLevelChanged.start();
		}

		update();
	}
}

void LevelMeterWidget::redrawTimerExpired() {
	// Check whether to clear the peak hold level
	if(m_peakHoldLevelChanged.elapsed() > PeakHoldLevelDuration && m_peakHoldLevel != m_decayedPeakLevel) {
		m_peakHoldLevel = m_decayedPeakLevel;

		update();
	}
}

float LevelMeterWidget::getNormalizedLevel(float levelDb) {
	static constexpr float maxValue = 6;
	static constexpr float minValue = -80;

	return (levelDb - minValue) / (maxValue - minValue);
}

void LevelMeterWidget::paintEvent(QPaintEvent* event) {
	Q_UNUSED(event)

	float peakLevel = getNormalizedLevel(m_decayedPeakLevel);
	float holdLevel = getNormalizedLevel(m_peakHoldLevel);

	QPainter painter(this);

	painter.setRenderHint(QPainter::Antialiasing, false);

	QRect frame = rect();
	QRect background(frame.left(), frame.top(), frame.width(), (1 - peakLevel) * frame.height());
	QRect bar(frame.left(), background.bottom(), frame.width(), frame.height() - background.bottom());
	QRect hold(frame.left(), frame.height() * (1 - holdLevel) - 2, frame.width(), 2);

	painter.fillRect(background, backgroundBrush);
	painter.fillRect(bar, levelBrush);

	if(m_peakHoldLevel < 1)
		painter.fillRect(hold, peakBrush);
	else
		painter.fillRect(hold, overLevelBrush);
}
