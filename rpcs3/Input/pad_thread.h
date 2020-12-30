#pragma once

#include "util/types.hpp"
#include "util/atomic.hpp"
#include "Emu/Io/pad_types.h"
#include "Emu/Io/pad_config_types.h"

#include <map>
#include <thread>
#include <mutex>
#include <string_view>
#include <string>

class PadHandlerBase;

class pad_thread
{
public:
	pad_thread(void* _curthread, void* _curwindow, std::string_view title_id); // void * instead of QThread * and QWindow * because of include in emucore
	~pad_thread();

	PadInfo& GetInfo() { return m_info; }
	auto& GetPads() { return m_pads; }
	void SetRumble(const u32 pad, u8 largeMotor, bool smallMotor);
	void Init();
	void SetIntercepted(bool intercepted);

	s32 AddLddPad();
	void UnregisterLddPad(u32 handle);

protected:
	void InitLddPad(u32 handle);
	void ThreadFunc();

	// List of all handlers
	std::map<pad_handler, std::shared_ptr<PadHandlerBase>> handlers;

	// Used for pad_handler::keyboard
	void *curthread;
	void *curwindow;

	PadInfo m_info{ 0, 0, false };
	std::array<std::shared_ptr<Pad>, CELL_PAD_MAX_PORT_NUM> m_pads;

	std::shared_ptr<std::thread> thread;

	u32 num_ldd_pad = 0;
};

namespace pad
{
	extern atomic_t<pad_thread*> g_current;
	extern std::recursive_mutex g_pad_mutex;
	extern std::string g_title_id;
	extern atomic_t<bool> g_enabled;
	extern atomic_t<bool> g_reset;
	extern atomic_t<bool> g_active;

	static inline class pad_thread* get_current_handler(bool relaxed = false)
	{
		if (relaxed)
		{
			return g_current.observe();
		}

		return ensure(g_current.load());
	}

	static inline void set_enabled(bool enabled)
	{
		g_enabled = enabled;
	}

	static inline void reset(std::string_view title_id)
	{
		g_title_id = title_id;
		g_reset = g_active.load();
	}

	static inline void SetIntercepted(bool intercepted)
	{
		std::lock_guard lock(g_pad_mutex);
		const auto handler = get_current_handler();
		handler->SetIntercepted(intercepted);
	}
}
