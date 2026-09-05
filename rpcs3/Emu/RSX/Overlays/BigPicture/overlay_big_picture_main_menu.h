#pragma once

#include "Emu/RSX/Overlays/HomeMenu/overlay_home_menu_sidebar_page.h"

#include <functional>
#include <string>

namespace rsx
{
	namespace overlays
	{
		struct big_picture_main_menu : public home_menu_sidebar_page
		{
			// on_game_selected is invoked once the user confirms "Start" on a game (path, title_id).
			big_picture_main_menu(s16 x, s16 y, u16 width, u16 height, home_menu_page* parent, std::function<void(std::string, std::string)> on_game_selected);
		};
	}
}
