#pragma once

#include "util/types.hpp"
#include "util/atomic.hpp"
#include "Utilities/lockless.h"
#include "util/shared_ptr.hpp"

extern lf_array<atomic_ptr<std::string>> g_progr_text_queue;

struct alignas(16) progress_dialog_string_t
{
	struct alignas(16) data_t
	{
		usz update_id = 0;
		u32 text_index = umax;
		u32 text_count = 0;
	};

	atomic_t<data_t> data{};

	shared_ptr<std::string> get_string_ptr() const noexcept;

	operator std::string() const noexcept
	{
		if (shared_ptr<std::string> ptr = get_string_ptr())
		{
			return *ptr;
		}

		return {};
	}

	explicit operator bool() const noexcept
	{
		return data.load().text_count != 0;
	}
};

enum system_progress_stop_state : u32
{
	stop_state_disabled = 0,
	stop_state_stopping,
	stop_state_continuous_savestate
};

extern progress_dialog_string_t g_progr_text;
extern atomic_t<u32> g_progr_ftotal;
extern atomic_t<u32> g_progr_fdone;
extern atomic_t<u64> g_progr_ftotal_bits;
extern atomic_t<u64> g_progr_fknown_bits;
extern atomic_t<u32> g_progr_ptotal;
extern atomic_t<u32> g_progr_pdone;
extern atomic_t<bool> g_system_progress_canceled;
extern atomic_t<system_progress_stop_state> g_system_progress_stopping;

// Initialize progress dialog (can be recursive)
class scoped_progress_dialog final
{
private:
	u32 m_text_index = 0;

public:
	scoped_progress_dialog(std::string text) noexcept;

	scoped_progress_dialog(const scoped_progress_dialog&) = delete;

	scoped_progress_dialog& operator=(const scoped_progress_dialog&) = delete;

	scoped_progress_dialog& operator=(std::string text) noexcept;

	~scoped_progress_dialog() noexcept;
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
