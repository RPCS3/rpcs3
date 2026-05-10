#pragma once

#include "util/atomic.hpp"

namespace input
{
	enum class active_mouse_and_keyboard : u32
	{
		pad,
		emulated
	};
	extern atomic_t<active_mouse_and_keyboard> g_active_mouse_and_keyboard;

	extern atomic_t<bool> g_pads_intercepted;
	extern atomic_t<bool> g_keyboards_intercepted;
	extern atomic_t<bool> g_mice_intercepted;

	void SetIntercepted(bool pads_intercepted, bool keyboards_intercepted, bool mice_intercepted, const char* func = __builtin_FUNCTION());
	void SetIntercepted(bool all_intercepted, const char* func = __builtin_FUNCTION());

	void set_mouse_and_keyboard(active_mouse_and_keyboard device);
	void toggle_mouse_and_keyboard();
}
