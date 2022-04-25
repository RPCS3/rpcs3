#pragma once

#include "util/atomic.hpp"

namespace input
{
	extern atomic_t<bool> g_pads_intercepted;
	extern atomic_t<bool> g_keyboards_intercepted;
	extern atomic_t<bool> g_mice_intercepted;

	void SetIntercepted(bool pads_intercepted, bool keyboards_intercepted, bool mice_intercepted, const char* func = __builtin_FUNCTION());
	void SetIntercepted(bool all_intercepted, const char* func = __builtin_FUNCTION());
}
