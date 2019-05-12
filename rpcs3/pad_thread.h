#pragma once

#include <map>
#include <thread>
#include <mutex>

#include "../Utilities/types.h"
#include "Emu/Io/PadHandler.h"

struct PadInfo
{
	u32 now_connect;
	u32 system_info;
};

class pad_thread
{
public:
	pad_thread(void* _curthread, void* _curwindow, const std::string& title_id = ""); // void * instead of QThread * and QWindow * because of include in emucore
	~pad_thread();

	PadInfo& GetInfo() { return m_info; }
	auto& GetPads() { return m_pads; }
	void SetRumble(const u32 pad, u8 largeMotor, bool smallMotor);
	void Init();
	void Reset(const std::string& title_id = "");
	void SetEnabled(bool enabled);
	void SetIntercepted(bool intercepted);

protected:
	void ThreadFunc();

	// List of all handlers
	std::map<pad_handler, std::shared_ptr<PadHandlerBase>> handlers;

	// Used for pad_handler::keyboard
	void *curthread;
	void *curwindow;

	PadInfo m_info{ 0, 0 };
	std::array<std::shared_ptr<Pad>, CELL_PAD_MAX_PORT_NUM> m_pads;

	atomic_t<bool> active{ false };
	atomic_t<bool> reset{ false };
	atomic_t<bool> is_enabled{ true };
	std::shared_ptr<std::thread> thread;
};

namespace pad
{
	extern atomic_t<pad_thread*> g_current;
	extern std::recursive_mutex g_pad_mutex;
	extern std::string g_title_id;

	static inline class pad_thread* get_current_handler()
	{
		return verify(HERE, g_current.load());
	}

	static inline void SetIntercepted(bool intercepted)
	{
		std::lock_guard lock(g_pad_mutex);
		const auto handler = get_current_handler();
		handler->SetIntercepted(intercepted);
	}
}
