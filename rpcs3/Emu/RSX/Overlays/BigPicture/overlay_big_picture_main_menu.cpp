#include "stdafx.h"
#include "overlay_big_picture_main_menu.h"
#include "overlay_big_picture_game_grid.h"
#include "Emu/RSX/Overlays/HomeMenu/overlay_home_menu_settings.h"
#include "Emu/System.h"

namespace rsx
{
	namespace overlays
	{
		big_picture_main_menu::big_picture_main_menu(s16 x, s16 y, u16 width, u16 height, home_menu_page* parent, std::function<void(std::string, std::string)> on_game_selected)
			: home_menu_sidebar_page(x, y, width, height, false, parent)
		{
			is_current_page = true;

			add_page(home_menu::fa_icon::home, std::make_shared<big_picture_game_grid>(x, y, width, height, this, std::move(on_game_selected)));

			add_page(home_menu::fa_icon::settings, std::make_shared<home_menu_settings>(x, y, width, height, false, this));

			add_item(home_menu::fa_icon::video, get_localized_string(localized_string_id::BIG_PICTURE_MENU_OPEN_INTERFACE), [](pad_button btn) -> page_navigation
			{
				if (btn != pad_button::cross) return page_navigation::stay;

				// Just bring the Qt window back, Big Picture Mode itself keeps running underneath it.
				Emu.CallFromMainThread([]()
				{
					Emu.GetCallbacks().show_main_window(true);
				});

				return page_navigation::stay;
			});

			add_item(home_menu::fa_icon::poweroff, get_localized_string(localized_string_id::BIG_PICTURE_MENU_EXIT), [](pad_button btn) -> page_navigation
			{
				if (btn != pad_button::cross) return page_navigation::stay;
				return page_navigation::exit;
			});

			apply_layout();
		}
	}
}
