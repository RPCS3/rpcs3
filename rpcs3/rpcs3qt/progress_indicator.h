#pragma once

#include <QWindow>

#ifdef HAS_QT_WIN_STUFF
#include <QWinTaskbarButton>
#endif

class progress_indicator
{
public:
	progress_indicator(int minimum, int maximum);
	~progress_indicator();

	void show(QWindow* window);
	void hide();

	int value() const;

	void set_value(int value);
	void set_range(int minimum, int maximum);
	void reset();
	void signal_failure();

private:

#ifdef HAS_QT_WIN_STUFF
	std::unique_ptr<QWinTaskbarButton> m_tb_button;
#else
	int m_value = 0;
	int m_minimum = 0;
	int m_maximum = 100;
#if HAVE_QTDBUS
	void update_progress(int progress, bool progress_visible, bool urgent);
#endif
#endif
};
