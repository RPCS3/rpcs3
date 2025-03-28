#include "stdafx.h"
#include "movie_item_base.h"

movie_item_base::movie_item_base() : qt_video_source()
{
	init_pointers();
}

movie_item_base::~movie_item_base()
{
	stop_movie();

	wait_for_icon_loading(true);
	wait_for_size_on_disk_loading(true);
}

void movie_item_base::init_pointers()
{
	m_icon_loading_aborted.reset(new atomic_t<bool>(false));
	m_size_on_disk_loading_aborted.reset(new atomic_t<bool>(false));
}

void movie_item_base::call_icon_load_func(int index)
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

void movie_item_base::set_icon_load_func(const icon_load_callback_t& func)
{
	wait_for_icon_loading(true);

	m_icon_loading = false;
	m_icon_load_callback = func;
	*m_icon_loading_aborted = false;
}

void movie_item_base::call_size_calc_func()
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

void movie_item_base::set_size_calc_func(const size_calc_callback_t& func)
{
	m_size_on_disk_loading = false;
	m_size_calc_callback = func;
	*m_size_on_disk_loading_aborted = false;
}

void movie_item_base::wait_for_icon_loading(bool abort)
{
	*m_icon_loading_aborted = abort;

	if (m_icon_load_thread && m_icon_load_thread->isRunning())
	{
		m_icon_load_thread->wait();
		m_icon_load_thread.reset();
	}
}

void movie_item_base::wait_for_size_on_disk_loading(bool abort)
{
	*m_size_on_disk_loading_aborted = abort;

	if (m_size_calc_thread && m_size_calc_thread->isRunning())
	{
		m_size_calc_thread->wait();
		m_size_calc_thread.reset();
	}
}
