#pragma once

#include "qt_video_source.h"

#include <QThread>

#include <memory>
#include <functional>

using icon_load_callback_t = std::function<void(int)>;
using size_calc_callback_t = std::function<void()>;

class movie_item_base : public qt_video_source
{
public:
	movie_item_base();
	virtual ~movie_item_base();

	void init_pointers();

	void call_icon_load_func(int index);
	void set_icon_load_func(const icon_load_callback_t& func);

	void call_size_calc_func();
	void set_size_calc_func(const size_calc_callback_t& func);

	void wait_for_icon_loading(bool abort);
	void wait_for_size_on_disk_loading(bool abort);

	bool icon_loading() const
	{
		return m_icon_loading;
	}

	bool size_on_disk_loading() const
	{
		return m_size_on_disk_loading;
	}

	[[nodiscard]] std::shared_ptr<atomic_t<bool>> icon_loading_aborted() const
	{
		return m_icon_loading_aborted;
	}

	[[nodiscard]] std::shared_ptr<atomic_t<bool>> size_on_disk_loading_aborted() const
	{
		return m_size_on_disk_loading_aborted;
	}

	shared_mutex pixmap_mutex;

private:
	std::unique_ptr<QThread> m_icon_load_thread;
	std::unique_ptr<QThread> m_size_calc_thread;
	atomic_t<bool> m_size_on_disk_loading = false;
	atomic_t<bool> m_icon_loading = false;
	size_calc_callback_t m_size_calc_callback = nullptr;
	icon_load_callback_t m_icon_load_callback = nullptr;

	std::shared_ptr<atomic_t<bool>> m_icon_loading_aborted;
	std::shared_ptr<atomic_t<bool>> m_size_on_disk_loading_aborted;
};
