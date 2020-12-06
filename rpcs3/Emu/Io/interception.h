#pragma once

namespace input
{
	extern atomic_t<bool> g_intercepted;

	void SetIntercepted(bool intercepted);
}
