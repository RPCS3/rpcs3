#pragma once

#include "util/types.hpp"
#include "util/atomic.hpp"
#include "Emu/Io/pad_types.h"
#include "Emu/Io/pad_config.h"
#include "Emu/Io/pad_config_types.h"
#include "Utilities/mutex.h"

#include <map>
#include <mutex>
#include <string_view>
#include <string>

class PadHandlerBase;

class pad_thread
{
public:
	pad_thread(void* curthread, void* curwindow, std::string_view title_id); // void * instead of QThread * and QWindow * because of include in emucore
	pad_thread(const pad_thread&) = delete;
	pad_thread& operator=(const pad_thread&) = delete;
	~pad_thread();

	void operator()();

	PadInfo& GetInfo() { return m_info; }
	std::array<std::shared_ptr<Pad>, CELL_PAD_MAX_PORT_NUM>& GetPads() { return m_pads; }
	void SetRumble(u32 pad, u8 large_motor, bool small_motor);
	void SetIntercepted(bool intercepted);

	s32 AddLddPad();
	void UnregisterLddPad(u32 handle);

	void open_home_menu();

	std::map<pad_handler, std::shared_ptr<PadHandlerBase>>& get_handlers() { return m_handlers; }

	static std::shared_ptr<PadHandlerBase> GetHandler(pad_handler type);
	static void InitPadConfig(cfg_pad& cfg, pad_handler type, std::shared_ptr<PadHandlerBase>& handler);

	static auto constexpr thread_name = "Pad Thread"sv;

protected:
	void Init();
	void InitLddPad(u32 handle, const u32* port_status);

	// List of all handlers
	std::map<pad_handler, std::shared_ptr<PadHandlerBase>> m_handlers;

	// Used for pad_handler::keyboard
	void* m_curthread = nullptr;
	void* m_curwindow = nullptr;

	PadInfo m_info{ 0, 0, false };
	std::array<std::shared_ptr<Pad>, CELL_PAD_MAX_PORT_NUM> m_pads{};
	std::array<bool, CELL_PAD_MAX_PORT_NUM> m_pads_connected{};

	u32 num_ldd_pad = 0;

private:
	void apply_copilots();
	void update_pad_states();

	u32 m_mask_start_press_to_resume = 0;
	u64 m_track_start_press_begin_timestamp = 0;
	bool m_resume_emulation_flag = false;
	bool m_ps_button_pressed = false;
	atomic_t<bool> m_home_menu_open = false;
};

namespace pad
{
	extern atomic_t<pad_thread*> g_pad_thread;
	extern shared_mutex g_pad_mutex;
	extern std::string g_title_id;
	extern atomic_t<bool> g_enabled;
	extern atomic_t<bool> g_reset;
	extern atomic_t<bool> g_started;
	extern atomic_t<bool> g_home_menu_requested;

	static inline class pad_thread* get_pad_thread(bool relaxed = false)
	{
		if (relaxed)
		{
			return g_pad_thread.observe();
		}

		return ensure(g_pad_thread.load());
	}

	static inline void set_enabled(bool enabled)
	{
		g_enabled = enabled;
	}

	static inline void reset(std::string_view title_id)
	{
		g_title_id = title_id;
		g_reset = true;
	}

	static inline void SetIntercepted(bool intercepted)
	{
		std::lock_guard lock(g_pad_mutex);
		const auto handler = get_pad_thread();
		handler->SetIntercepted(intercepted);
	}
}
