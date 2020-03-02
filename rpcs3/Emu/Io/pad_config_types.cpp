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
#ifdef _WIN32
		case pad_handler::xinput: return "XInput";
		case pad_handler::mm: return "MMJoystick";
#endif
#ifdef HAVE_LIBEVDEV
		case pad_handler::evdev: return "Evdev";
#endif
		}

		return unknown;
	});
}
