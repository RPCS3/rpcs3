#pragma once

#include "util/types.hpp"

enum class pad_handler
{
	null,
	keyboard,
	ds3,
	ds4,
	dualsense,
#ifdef _WIN32
	xinput,
	mm,
#endif
#ifdef HAVE_LIBEVDEV
	evdev,
#endif
#ifdef __APPLE__
	gamecontroller,
#endif
};

enum class mouse_movement_mode : s32
{
	relative = 0,
	absolute = 1
};

struct PadInfo
{
	u32 now_connect;
	u32 system_info;
	bool ignore_input;
};
