#pragma once

class progress_indicator
{
public:
	progress_indicator(int minimum, int maximum);
	~progress_indicator();

	int value() const;

	void set_value(int value);
	void set_range(int minimum, int maximum);
	void reset();
	void signal_failure();

private:
	int m_value = 0;
	int m_minimum = 0;
	int m_maximum = 100;
#if HAVE_QTDBUS
	void update_progress(int progress, bool progress_visible, bool urgent);
#endif
};
