#pragma once

#include <QLabel>
#include <QTimer>

class auto_scroll_label : public QLabel
{
public:
	auto_scroll_label(QWidget* parent = nullptr);

	void setText(const QString& text);

protected:
	void paintEvent(QPaintEvent* event) override;
	void resizeEvent(QResizeEvent* event) override;

private slots:
	void update_scroll();

private:
	void update_scroll_offset();
	void update_text_height();

	QTimer m_timer;

	static constexpr int m_timer_interval = 30; // ms
	static constexpr int m_pause_tick_count = 50; // ~1.5 seconds at 30 ms

	int m_scroll_offset = 0;
	int m_text_height = 0;
	int m_pause_ticks = 0;
	bool m_at_end = false;
};
