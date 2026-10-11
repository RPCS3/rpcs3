#include "stdafx.h"
#include "auto_scroll_label.h"

#include <QPainter>

auto_scroll_label::auto_scroll_label(QWidget* parent)
	: QLabel(parent)
{
	connect(&m_timer, &QTimer::timeout, this, &auto_scroll_label::update_scroll);

	m_timer.start(m_timer_interval);
}

void auto_scroll_label::setText(const QString& text)
{
	if (text == this->text()) return;

	QLabel::setText(text);

	update_text_height();
	m_scroll_offset = 0;
	m_pause_ticks = 0;
}

void auto_scroll_label::paintEvent(QPaintEvent* /*event*/)
{
	QPainter p(this);

	p.setFont(font());
	p.setPen(palette().windowText().color());
	p.setClipRect(rect());

	QRect r = contentsRect();
	r.setHeight(m_text_height);
	r.translate(0, -m_scroll_offset);

	p.drawText(r, alignment() | Qt::TextWordWrap, text());
}

void auto_scroll_label::resizeEvent(QResizeEvent* event)
{
	QLabel::resizeEvent(event);
	update_text_height();
}

void auto_scroll_label::update_text_height()
{
	const QFontMetrics fm(font());
	const QRect r = fm.boundingRect(QRect(0, 0, contentsRect().width(), INT_MAX), alignment() | Qt::TextWordWrap, text());

	m_text_height = r.height();
}

void auto_scroll_label::update_scroll()
{
	const int old_scroll_offset = m_scroll_offset;
	update_scroll_offset();

	if (isVisible() && m_scroll_offset != old_scroll_offset)
	{
		update();
	}
}

void auto_scroll_label::update_scroll_offset()
{
	if (!isVisible() || m_text_height <= height())
	{
		m_scroll_offset = 0;
		m_pause_ticks = 0;
		return;
	}

	if (m_pause_ticks < m_pause_tick_count)
	{
		m_pause_ticks++;
		return;
	}

	const int max_offset = m_text_height - height();

	if (++m_scroll_offset >= max_offset)
	{
		m_pause_ticks = 0;

		if (m_at_end)
		{
			m_scroll_offset = 0;
		}

		m_at_end = !m_at_end;
	}
}
