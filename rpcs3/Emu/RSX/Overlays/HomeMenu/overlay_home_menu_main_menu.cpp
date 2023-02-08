#include "stdafx.h"
#include "overlay_home_menu_main_menu.h"
#include "overlay_home_menu_components.h"
#include "Emu/System.h"

extern atomic_t<bool> g_user_asked_for_recording;
extern atomic_t<bool> g_user_asked_for_screenshot;

namespace rsx
{
	namespace overlays
	{
		home_menu_main_menu::home_menu_main_menu(u16 x, u16 y, u16 width, u16 height, bool use_separators, home_menu_page* parent)
			: home_menu_page(x, y, width, height, use_separators, parent, get_localized_string(localized_string_id::HOME_MENU_TITLE))
		{
			is_current_page = true;

			std::unique_ptr<overlay_element> resume = std::make_unique<home_menu_entry>(get_localized_string(localized_string_id::HOME_MENU_RESUME));
			add_item(resume, [](pad_button btn) -> page_navigation
			{
				if (btn != pad_button::cross) return page_navigation::stay;

				rsx_log.notice("User selected resume in home menu");
				return page_navigation::exit;
			});

			add_page(std::make_shared<home_menu_settings>(x, y, width, height, use_separators, this));

			std::unique_ptr<overlay_element> screenshot = std::make_unique<home_menu_entry>(get_localized_string(localized_string_id::HOME_MENU_SCREENSHOT));
			add_item(screenshot, [](pad_button btn) -> page_navigation
			{
				if (btn != pad_button::cross) return page_navigation::stay;

				rsx_log.notice("User selected screenshot in home menu");
				g_user_asked_for_screenshot = true;
				return page_navigation::exit;
			});

			std::unique_ptr<overlay_element> recording = std::make_unique<home_menu_entry>(get_localized_string(localized_string_id::HOME_MENU_RECORDING));
			add_item(recording, [](pad_button btn) -> page_navigation
			{
				if (btn != pad_button::cross) return page_navigation::stay;

				rsx_log.notice("User selected recording in home menu");
				g_user_asked_for_recording = true;
				return page_navigation::exit;
			});

			std::unique_ptr<overlay_element> exit_game = std::make_unique<home_menu_entry>(get_localized_string(localized_string_id::HOME_MENU_EXIT_GAME));
			add_item(exit_game, [](pad_button btn) -> page_navigation
			{
				if (btn != pad_button::cross) return page_navigation::stay;

				rsx_log.notice("User selected exit game in home menu");
				Emu.CallFromMainThread([]
				{
					Emu.GracefulShutdown(false, true);
				});
				return page_navigation::stay;
			});

			apply_layout();
		}
	}
}
