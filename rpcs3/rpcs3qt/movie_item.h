#pragma once

#include <QTableWidgetItem>
#include <QMovie>
#include <QObject>

#include <functional>

using icon_callback_t = std::function<void(int)>;

class movie_item : public QTableWidgetItem
{
public:
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
		if (!std::exchange(m_active, active) && active && m_movie)
		{
			m_movie->jumpToFrame(1);
			m_movie->start();
		}
	}

	[[nodiscard]] bool get_active() const
	{
		return m_active;
	}

	[[nodiscard]] QMovie* movie() const
	{
		return m_movie;
	}

	void init_movie(const QString& path)
	{
		if (path.isEmpty() || !m_icon_callback) return;

		if (QMovie* movie = new QMovie(path); movie && movie->isValid())
		{
			m_movie = movie;
		}
		else
		{
			delete movie;
			return;
		}

		QObject::connect(m_movie, &QMovie::frameChanged, m_movie, m_icon_callback);
	}

	void set_icon_func(const icon_callback_t& func)
	{
		m_icon_callback = func;

		if (m_icon_callback)
		{
			m_icon_callback(0);
		}
	}

private:
	QMovie* m_movie = nullptr;
	bool m_active = false;
	icon_callback_t m_icon_callback = nullptr;
};
