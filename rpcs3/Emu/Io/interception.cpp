#include "stdafx.h"
#include "interception.h"
#include "KeyboardHandler.h"
#include "MouseHandler.h"
#include "Input/pad_thread.h"
#include "Emu/IdManager.h"

namespace input
{
	atomic_t<bool> g_pads_intercepted{false};
	atomic_t<bool> g_keyboards_intercepted{false};
	atomic_t<bool> g_mice_intercepted{false};

	void SetIntercepted(bool pads_intercepted, bool keyboards_intercepted, bool mice_intercepted)
	{
		g_pads_intercepted = pads_intercepted;
		g_keyboards_intercepted = keyboards_intercepted;
		g_mice_intercepted = mice_intercepted;

		pad::SetIntercepted(pads_intercepted);

		if (const auto handler = g_fxo->try_get<KeyboardHandlerBase>())
		{
			handler->SetIntercepted(keyboards_intercepted);
		}

		if (const auto handler = g_fxo->try_get<MouseHandlerBase>())
		{
			handler->SetIntercepted(mice_intercepted);
		}
	}

	void SetIntercepted(bool all_intercepted)
	{
		SetIntercepted(all_intercepted, all_intercepted, all_intercepted);
	}
}
