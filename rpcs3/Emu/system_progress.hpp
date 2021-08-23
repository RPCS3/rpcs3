#pragma once

#include "util/types.hpp"
#include "util/atomic.hpp"

extern atomic_t<const char*> g_progr;
extern atomic_t<u32> g_progr_ftotal;
extern atomic_t<u32> g_progr_fdone;
extern atomic_t<u32> g_progr_ptotal;
extern atomic_t<u32> g_progr_pdone;
extern atomic_t<bool> g_system_progress_canceled;

// Initialize progress dialog (can be recursive)
class scoped_progress_dialog final
{
	// Saved previous value
	const char* const m_prev;

public:
	scoped_progress_dialog(const char* text)
		: m_prev(g_progr.exchange(text ? text : ""))
	{
	}

	scoped_progress_dialog(const scoped_progress_dialog&) = delete;

	scoped_progress_dialog& operator=(const scoped_progress_dialog&) = delete;

	~scoped_progress_dialog()
	{
		g_progr.release(m_prev);
	}
};

struct progress_dialog_server
{
	void operator()();
	~progress_dialog_server();

	static auto constexpr thread_name = "Progress Dialog Server"sv;
};
