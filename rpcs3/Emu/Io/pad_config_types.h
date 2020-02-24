#pragma once

#include "Utilities/types.h"

enum class pad_handler
{
	null,
	keyboard,
	ds3,
	ds4,
#ifdef _WIN32
	xinput,
	mm,
#endif
#ifdef HAVE_LIBEVDEV
	evdev,
#endif
};

struct PadInfo
{
	u32 now_connect;
	u32 system_info;
	bool ignore_input;
};
