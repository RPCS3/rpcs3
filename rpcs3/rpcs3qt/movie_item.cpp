#include "stdafx.h"
#include "movie_item.h"

movie_item::movie_item() : QTableWidgetItem()
{
	init_pointers();
}

movie_item::movie_item(const QString& text, int type) : QTableWidgetItem(text, type)
{
	init_pointers();
}

movie_item::movie_item(const QIcon& icon, const QString& text, int type) : QTableWidgetItem(icon, text, type)
{
	init_pointers();
}

movie_item::~movie_item()
{
	if (m_movie)
	{
		m_movie->stop();
	}

	wait_for_icon_loading(true);
	wait_for_size_on_disk_loading(true);
}

void movie_item::init_pointers()
{
	m_icon_loading_aborted.reset(new atomic_t<bool>(false));
	m_size_on_disk_loading_aborted.reset(new atomic_t<bool>(false));
}

void movie_item::set_active(bool active)
{
	if (!std::exchange(m_active, active) && active && m_movie)
	{
		m_movie->jumpToFrame(1);
		m_movie->start();
	}
}

void movie_item::init_movie(const QString& path)
{
	if (path.isEmpty() || !m_icon_callback) return;

	m_movie.reset(new QMovie(path));

	if (!m_movie->isValid())
	{
		m_movie.reset();
		return;
	}

	QObject::connect(m_movie.get(), &QMovie::frameChanged, m_movie.get(), m_icon_callback);
}

void movie_item::call_icon_func() const
{
	if (m_icon_callback)
	{
		m_icon_callback(0);
	}
}

void movie_item::set_icon_func(const icon_callback_t& func)
{
	m_icon_callback = func;
	call_icon_func();
}

void movie_item::call_icon_load_func(int index)
{
	if (!m_icon_load_callback || m_icon_loading || m_icon_loading_aborted->load())
	{
		return;
	}

	wait_for_icon_loading(true);

	*m_icon_loading_aborted = false;
	m_icon_loading = true;
	m_icon_load_thread.reset(QThread::create([this, index]()
	{
		if (m_icon_load_callback)
		{
			m_icon_load_callback(index);
		}
	}));
	m_icon_load_thread->start();
}

void movie_item::set_icon_load_func(const icon_load_callback_t& func)
{
	wait_for_icon_loading(true);

	m_icon_loading = false;
	m_icon_load_callback = func;
	*m_icon_loading_aborted = false;
}

void movie_item::call_size_calc_func()
{
	if (!m_size_calc_callback || m_size_on_disk_loading || m_size_on_disk_loading_aborted->load())
	{
		return;
	}

	wait_for_size_on_disk_loading(true);

	*m_size_on_disk_loading_aborted = false;
	m_size_on_disk_loading = true;
	m_size_calc_thread.reset(QThread::create([this]()
	{
		if (m_size_calc_callback)
		{
			m_size_calc_callback();
		}
	}));
	m_size_calc_thread->start();
}

void movie_item::set_size_calc_func(const size_calc_callback_t& func)
{
	m_size_on_disk_loading = false;
	m_size_calc_callback = func;
	*m_size_on_disk_loading_aborted = false;
}

void movie_item::wait_for_icon_loading(bool abort)
{
	*m_icon_loading_aborted = abort;

	if (m_icon_load_thread && m_icon_load_thread->isRunning())
	{
		m_icon_load_thread->wait();
		m_icon_load_thread.reset();
	}
}

void movie_item::wait_for_size_on_disk_loading(bool abort)
{
	*m_size_on_disk_loading_aborted = abort;

	if (m_size_calc_thread && m_size_calc_thread->isRunning())
	{
		m_size_calc_thread->wait();
		m_size_calc_thread.reset();
	}
}
