#include "stdafx.h"
#include "interception.h"
#include "KeyboardHandler.h"
#include "MouseHandler.h"
#include "Input/pad_thread.h"
#include "Emu/IdManager.h"
#include "Emu/RSX/Overlays/overlay_message.h"
#include "Emu/system_config.h"

LOG_CHANNEL(input_log, "Input");

template <>
void fmt_class_string<input::active_mouse_and_keyboard>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](input::active_mouse_and_keyboard value)
	{
		switch (value)
		{
		case input::active_mouse_and_keyboard::emulated: return "emulated";
		case input::active_mouse_and_keyboard::pad: return "pad";
		}

		return unknown;
	});
}

namespace input
{
	atomic_t<active_mouse_and_keyboard> g_active_mouse_and_keyboard{active_mouse_and_keyboard::emulated};
	atomic_t<bool> g_pads_intercepted{false};
	atomic_t<bool> g_keyboards_intercepted{false};
	atomic_t<bool> g_mice_intercepted{false};

	void SetIntercepted(bool pads_intercepted, bool keyboards_intercepted, bool mice_intercepted, const char* func)
	{
		input_log.notice("SetIntercepted: pads=%d, keyboards=%d, mice=%d, src=%s)", pads_intercepted, keyboards_intercepted, mice_intercepted, func);

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

	void SetIntercepted(bool all_intercepted, const char* func)
	{
		SetIntercepted(all_intercepted, all_intercepted, all_intercepted, func);
	}

	static void show_mouse_and_keyboard_overlay()
	{
		if (!g_cfg.misc.show_mouse_and_keyboard_toggle_hint)
			return;

		const localized_string_id id = g_active_mouse_and_keyboard == active_mouse_and_keyboard::emulated
			? localized_string_id::RSX_OVERLAYS_MOUSE_AND_KEYBOARD_EMULATED
			: localized_string_id::RSX_OVERLAYS_MOUSE_AND_KEYBOARD_PAD;
		rsx::overlays::queue_message(get_localized_string(id), 3'000'000);
	}

	void set_mouse_and_keyboard(active_mouse_and_keyboard device)
	{
		// Always log
		input_log.notice("set_mouse_and_keyboard: device=%s", device);

		if (g_active_mouse_and_keyboard != device)
		{
			g_active_mouse_and_keyboard = device;
			show_mouse_and_keyboard_overlay();
		}
	}

	void toggle_mouse_and_keyboard()
	{
		g_active_mouse_and_keyboard = g_active_mouse_and_keyboard == active_mouse_and_keyboard::emulated ? active_mouse_and_keyboard::pad : active_mouse_and_keyboard::emulated;
		input_log.notice("toggle_mouse_and_keyboard: device=%s", g_active_mouse_and_keyboard.load());
		show_mouse_and_keyboard_overlay();
	}
}
