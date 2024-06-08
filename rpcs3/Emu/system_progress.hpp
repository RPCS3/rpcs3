#pragma once

#include "util/types.hpp"
#include "util/atomic.hpp"

struct alignas(16) progress_dialog_string_t
{
	const char* m_text;
	u32 m_user_count;
	u32 m_update_id;

	operator const char*() const noexcept
	{
		return m_text;
	}
};

extern atomic_t<progress_dialog_string_t> g_progr;
extern atomic_t<u32> g_progr_ftotal;
extern atomic_t<u32> g_progr_fdone;
extern atomic_t<u64> g_progr_ftotal_bits;
extern atomic_t<u64> g_progr_fknown_bits;
extern atomic_t<u32> g_progr_ptotal;
extern atomic_t<u32> g_progr_pdone;
extern atomic_t<bool> g_system_progress_canceled;
extern atomic_t<bool> g_system_progress_stopping;

// Initialize progress dialog (can be recursive)
class scoped_progress_dialog final
{
	// Saved previous value
	const char* m_prev;
	u32 m_prev_id;
	u32 m_id;

public:
	scoped_progress_dialog(const char* text) noexcept
	{
		std::tie(m_prev, m_prev_id, m_id) = g_progr.atomic_op([text = ensure(text)](progress_dialog_string_t& progr)
		{
			const char* old = progr.m_text;
			progr.m_user_count++;
			progr.m_update_id++;
			progr.m_text = text;

			ensure(progr.m_user_count > 1 || !old); // Ensure it was nullptr before first use
			return std::make_tuple(old, progr.m_update_id - 1, progr.m_update_id);
		});
	}

	scoped_progress_dialog(const scoped_progress_dialog&) = delete;

	scoped_progress_dialog& operator=(const scoped_progress_dialog&) = delete;

	scoped_progress_dialog& operator=(const char* text) noexcept
	{
		// This method is destroying the previous value and replacing it with a new one
		std::tie(m_prev, m_prev_id, m_id) = g_progr.atomic_op([this, text = ensure(text)](progress_dialog_string_t& progr)
		{
			if (m_id == progr.m_update_id)
			{
				progr.m_update_id = m_prev_id;
				progr.m_text = m_prev;
			}

			const char* old = progr.m_text;
			progr.m_text = text;
			progr.m_update_id++;

			ensure(progr.m_user_count > 0);
			return std::make_tuple(old, progr.m_update_id - 1, progr.m_update_id);
		});

		return *this;
	}

	~scoped_progress_dialog() noexcept
	{
		g_progr.atomic_op([this](progress_dialog_string_t& progr)
		{
			if (progr.m_user_count-- == 1)
			{
				// Clean text only on last user
				progr.m_text = nullptr;
				progr.m_update_id = 0;
			}
			else if (m_id == progr.m_update_id)
			{
				// Restore text only if no other updates were made by other threads
				progr.m_text = ensure(m_prev);
				progr.m_update_id = m_prev_id;
			}
		});
	}
};

struct progress_dialog_server
{
	void operator()();
	~progress_dialog_server();

	static constexpr auto thread_name = "Progress Dialog Server"sv;
};

struct progress_dialog_workaround
{
	// We don't want to show the native dialog during gameplay.
	atomic_t<bool> show_overlay_message_only = false;
};
