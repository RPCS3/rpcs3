#pragma once

#include <QTableWidgetItem>
#include <QMovie>
#include <QObject>

class movie_item : public QTableWidgetItem
{
private:
	bool m_active = false;

public:
	QMovie* m_movie = nullptr;

	movie_item() : QTableWidgetItem()
	{
	}
	movie_item(const QString& text, int type = Type) : QTableWidgetItem(text, type)
	{
	}
	movie_item(const QIcon& icon, const QString& text, int type = Type) : QTableWidgetItem(icon, text, type)
	{
	}

	~movie_item()
	{
		if (m_movie)
		{
			m_movie->stop();
			delete m_movie;
		}
	}

	void set_active(bool active)
	{
		const bool was_active = m_active;

		m_active = active;

		if (active != was_active && m_movie)
		{
			if (active)
			{
				m_movie->jumpToFrame(1);
				m_movie->start();
			}
		}
	}

	bool get_active()
	{
		return m_active;
	}

	void init_movie()
	{
		static int i = 0;
		QString path;
		switch (i++ % 5)
		{
		case 0:
			path = ":/Icons/nggyu.gif";
			break;
		case 1:
			path = ":/Icons/nggyu_2.gif";
			break;
		case 2:
			path = ":/Icons/nggyu_3.gif";
			break;
		case 3:
			path = ":/Icons/nggyu_sw.gif";
			break;
		case 4:
		default:
			path = ":/Icons/nggyu_pixel.gif";
			break;
		}
		m_movie = new QMovie(path);
		m_movie->start();
	}
};
