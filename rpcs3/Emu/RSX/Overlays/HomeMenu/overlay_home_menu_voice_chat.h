#pragma once

#include "overlay_home_menu_settings.h"

namespace rsx
{
	namespace overlays
	{
		struct home_menu_voice_chat : public home_menu_settings_page
		{
			home_menu_voice_chat(s16 x, s16 y, u16 width, u16 height, bool use_separators, home_menu_page* parent);
		};
	}
}
