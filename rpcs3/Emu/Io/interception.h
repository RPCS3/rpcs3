#pragma once

#include "util/types.hpp"
#include "util/atomic.hpp"

namespace input
{
	extern atomic_t<bool> g_intercepted;

	void SetIntercepted(bool intercepted);
}
