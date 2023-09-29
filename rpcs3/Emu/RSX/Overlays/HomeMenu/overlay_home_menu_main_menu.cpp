#include "stdafx.h"
#include "overlay_home_menu_main_menu.h"
#include "overlay_home_menu_components.h"
#include "Emu/System.h"
#include "Emu/system_config.h"

extern atomic_t<bool> g_user_asked_for_recording;
extern atomic_t<bool> g_user_asked_for_screenshot;
extern bool boot_last_savestate(bool testing);

namespace rsx
{
	namespace overlays
	{
		home_menu_main_menu::home_menu_main_menu(u16 x, u16 y, u16 width, u16 height, bool use_separators, home_menu_page* parent)
			: home_menu_page(x, y, width, height, use_separators, parent, get_localized_string(localized_string_id::HOME_MENU_TITLE))
		{
			is_current_page = true;

			m_message_box = std::make_shared<home_menu_message_box>(x, y, width, height);
			m_config_changed = std::make_shared<bool>(g_backup_cfg.to_string() != g_cfg.to_string());

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

			const bool suspend_mode = g_cfg.savestate.suspend_emu.get();

			std::unique_ptr<overlay_element> save_state = std::make_unique<home_menu_entry>(get_localized_string(suspend_mode ? localized_string_id::HOME_MENU_SAVESTATE_AND_EXIT : localized_string_id::HOME_MENU_SAVESTATE));
			add_item(save_state, [suspend_mode](pad_button btn) -> page_navigation
			{
				if (btn != pad_button::cross) return page_navigation::stay;

				rsx_log.notice("User selected savestate in home menu");

				Emu.CallFromMainThread([suspend_mode]()
				{
					if (!suspend_mode)
					{
						Emu.after_kill_callback = []()
						{
							Emu.Restart();
						};
					}

					Emu.Kill(false, true);
				});

				return page_navigation::exit;
			});

			if (!suspend_mode && boot_last_savestate(true))
			{
				std::unique_ptr<overlay_element> reload_state = std::make_unique<home_menu_entry>(get_localized_string(localized_string_id::HOME_MENU_RELOAD_SAVESTATE));
				add_item(reload_state, [](pad_button btn) -> page_navigation
				{
					if (btn != pad_button::cross) return page_navigation::stay;

					rsx_log.notice("User selected reload savestate in home menu");

					Emu.CallFromMainThread([]()
					{
						boot_last_savestate(false);
					});

					return page_navigation::exit;
				});
			}

			std::unique_ptr<overlay_element> restart = std::make_unique<home_menu_entry>(get_localized_string(localized_string_id::HOME_MENU_RESTART));
			add_item(restart, [](pad_button btn) -> page_navigation
			{
				if (btn != pad_button::cross) return page_navigation::stay;

				rsx_log.notice("User selected restart in home menu");
				
				Emu.CallFromMainThread([]()
				{
					Emu.Restart(false);
				});
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
