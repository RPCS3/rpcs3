#include "stdafx.h"
#include "pad_config_types.h"

template <>
void fmt_class_string<pad_handler>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](pad_handler value)
	{
		switch (value)
		{
		case pad_handler::null: return "Null";
		case pad_handler::keyboard: return "Keyboard";
		case pad_handler::ds3: return "DualShock 3";
		case pad_handler::ds4: return "DualShock 4";
		case pad_handler::dualsense: return "DualSense";
		case pad_handler::skateboard: return "Skateboard";
		case pad_handler::move: return "PS Move";
#ifdef _WIN32
		case pad_handler::xinput: return "XInput";
		case pad_handler::mm: return "MMJoystick";
#endif
#ifdef HAVE_SDL3
		case pad_handler::sdl: return "SDL";
#endif
#ifdef HAVE_LIBEVDEV
		case pad_handler::evdev: return "Evdev";
#endif
		}

		return unknown;
	});
}

template <>
void fmt_class_string<mouse_movement_mode>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](mouse_movement_mode value)
	{
		switch (value)
		{
		case mouse_movement_mode::relative: return "Relative";
		case mouse_movement_mode::absolute: return "Absolute";
		}

		return unknown;
	});
}
