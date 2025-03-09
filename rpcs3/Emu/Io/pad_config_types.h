#pragma once

#include "util/types.hpp"

enum class pad_handler
{
	null,
	keyboard,
	ds3,
	ds4,
	dualsense,
	skateboard,
	move,
#ifdef _WIN32
	xinput,
	mm,
#endif
#ifdef HAVE_SDL3
	sdl,
#endif
#ifdef HAVE_LIBEVDEV
	evdev,
#endif
};

enum class mouse_movement_mode : s32
{
	relative = 0,
	absolute = 1
};

struct PadInfo
{
	u32 now_connect = 0;
	u32 system_info = 0;
	bool ignore_input = false;
};
