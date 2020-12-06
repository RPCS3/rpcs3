#include "stdafx.h"
#include "interception.h"
#include "KeyboardHandler.h"
#include "MouseHandler.h"
#include "Input/pad_thread.h"
#include "Emu/IdManager.h"

namespace input
{
	atomic_t<bool> g_intercepted{false};

	void SetIntercepted(bool intercepted)
	{
		g_intercepted = intercepted;

		pad::SetIntercepted(intercepted);

		if (const auto handler = g_fxo->get<KeyboardHandlerBase>())
		{
			handler->SetIntercepted(intercepted);
		}

		if (const auto handler = g_fxo->get<MouseHandlerBase>())
		{
			handler->SetIntercepted(intercepted);
		}
	}
}
