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
		case input::active_mouse_and_keyboard::pad: return "pad";
		case input::active_mouse_and_keyboard::emulated: return "emulated";
		case input::active_mouse_and_keyboard::emulated_mouse_only: return "emulated_mouse_only";
		case input::active_mouse_and_keyboard::emulated_kb_only: return "emulated_kb_only";
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

		localized_string_id id = localized_string_id::RSX_OVERLAYS_MOUSE_AND_KEYBOARD_PAD;
		switch (g_active_mouse_and_keyboard.load())
		{
		case input::active_mouse_and_keyboard::pad: id = localized_string_id::RSX_OVERLAYS_MOUSE_AND_KEYBOARD_PAD; break;
		case input::active_mouse_and_keyboard::emulated: id = localized_string_id::RSX_OVERLAYS_MOUSE_AND_KEYBOARD_EMULATED; break;
		case input::active_mouse_and_keyboard::emulated_mouse_only: id = localized_string_id::RSX_OVERLAYS_MOUSE_AND_KEYBOARD_EMULATED_MOUSE_ONLY; break;
		case input::active_mouse_and_keyboard::emulated_kb_only: id = localized_string_id::RSX_OVERLAYS_MOUSE_AND_KEYBOARD_EMULATED_KB_ONLY; break;
		}

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
		switch (g_active_mouse_and_keyboard.load())
		{
		case input::active_mouse_and_keyboard::pad: g_active_mouse_and_keyboard = active_mouse_and_keyboard::emulated; break;
		case input::active_mouse_and_keyboard::emulated: g_active_mouse_and_keyboard = active_mouse_and_keyboard::emulated_mouse_only; break;
		case input::active_mouse_and_keyboard::emulated_mouse_only: g_active_mouse_and_keyboard = active_mouse_and_keyboard::emulated_kb_only; break;
		case input::active_mouse_and_keyboard::emulated_kb_only: g_active_mouse_and_keyboard = active_mouse_and_keyboard::pad; break;
		}
		input_log.notice("toggle_mouse_and_keyboard: device=%s", g_active_mouse_and_keyboard.load());
		show_mouse_and_keyboard_overlay();
	}
}
