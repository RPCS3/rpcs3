#pragma once

#include <atomic>

namespace input
{
	extern std::atomic<bool> g_intercepted;

	void SetIntercepted(bool intercepted);
}
