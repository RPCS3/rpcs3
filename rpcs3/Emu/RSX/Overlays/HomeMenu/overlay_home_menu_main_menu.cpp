#include "stdafx.h"
#include "overlay_home_menu_main_menu.h"
#include "overlay_home_menu_components.h"
#include "overlay_home_menu_settings.h"
#include "overlay_home_menu_savestate.h"
#include "Emu/RSX/Overlays/FriendsList/overlay_friends_list_dialog.h"
#include "Emu/RSX/Overlays/Trophies/overlay_trophy_list_dialog.h"
#include "Emu/RSX/Overlays/overlay_manager.h"
#include "Emu/System.h"
#include "Emu/system_config.h"
#include "Emu/Cell/Modules/sceNpTrophy.h"

extern atomic_t<bool> g_user_asked_for_recording;

namespace rsx
{
	namespace overlays
	{
		home_menu_main_menu::home_menu_main_menu(s16 x, s16 y, u16 width, u16 height, bool use_separators, home_menu_page* parent)
			: home_menu_page(x, y, width, height, use_separators, parent, get_localized_string(localized_string_id::HOME_MENU_TITLE))
		{
			is_current_page = true;

			m_message_box = std::make_shared<home_menu_message_box>(x, y, width, height);
			m_message_box->visible = false;

			m_config_changed = std::make_shared<bool>(g_backup_cfg.to_string() != g_cfg.to_string());

			std::unique_ptr<overlay_element> resume = std::make_unique<home_menu_entry>(get_localized_string(localized_string_id::HOME_MENU_RESUME));
			add_item(resume, [](pad_button btn) -> page_navigation
			{
				if (btn != pad_button::cross) return page_navigation::stay;

				rsx_log.notice("User selected resume in home menu");
				return page_navigation::exit;
			});

			add_page(std::make_shared<home_menu_settings>(x, y, width, height, use_separators, this));

			if (rsx::overlays::friends_list_dialog::rpcn_configured())
			{
				std::unique_ptr<overlay_element> friends = std::make_unique<home_menu_entry>(get_localized_string(localized_string_id::HOME_MENU_FRIENDS));
				add_item(friends, [](pad_button btn) -> page_navigation
				{
					if (btn != pad_button::cross) return page_navigation::stay;

					rsx_log.notice("User selected friends in home menu");
					Emu.CallFromMainThread([]()
					{
						if (auto manager = g_fxo->try_get<rsx::overlays::display_manager>())
						{
							const error_code result = manager->create<rsx::overlays::friends_list_dialog>()->show(true, [](s32 status)
							{
								rsx_log.notice("Closing friends list with status %d", status);
							});

							(result ? rsx_log.error : rsx_log.notice)("Opened friends list with result %d", s32{result});
						}
					});
					return page_navigation::stay;
				});
			}
			else
			{
				rsx_log.notice("Friends list hidden in home menu. RPCN is not configured.");
			}

			// get current trophy name for trophy list overlay
			std::string trop_name;
			{
				current_trophy_name& current_id = g_fxo->get<current_trophy_name>();
				std::lock_guard lock(current_id.mtx);
				trop_name = current_id.name;
			}
			if (!trop_name.empty())
			{
				std::unique_ptr<overlay_element> trophies = std::make_unique<home_menu_entry>(get_localized_string(localized_string_id::HOME_MENU_TROPHIES));
				add_item(trophies, [trop_name = std::move(trop_name)](pad_button btn) -> page_navigation
				{
					if (btn != pad_button::cross) return page_navigation::stay;

					rsx_log.notice("User selected trophies in home menu");
					Emu.CallFromMainThread([trop_name = std::move(trop_name)]()
					{
						if (auto manager = g_fxo->try_get<rsx::overlays::display_manager>())
						{
							manager->create<rsx::overlays::trophy_list_dialog>()->show(trop_name);
						}
					});
					return page_navigation::stay;
				});
			}

			std::unique_ptr<overlay_element> screenshot = std::make_unique<home_menu_entry>(get_localized_string(localized_string_id::HOME_MENU_SCREENSHOT));
			add_item(screenshot, [](pad_button btn) -> page_navigation
			{
				if (btn != pad_button::cross) return page_navigation::stay;

				rsx_log.notice("User selected screenshot in home menu");
				return page_navigation::exit_for_screenshot;
			});

			std::unique_ptr<overlay_element> recording = std::make_unique<home_menu_entry>(get_localized_string(localized_string_id::HOME_MENU_RECORDING));
			add_item(recording, [](pad_button btn) -> page_navigation
			{
				if (btn != pad_button::cross) return page_navigation::stay;

				rsx_log.notice("User selected recording in home menu");
				g_user_asked_for_recording = true;
				return page_navigation::exit;
			});

			add_page(std::make_shared<home_menu_savestate>(x, y, width, height, use_separators, this));

			std::unique_ptr<overlay_element> restart = std::make_unique<home_menu_entry>(get_localized_string(localized_string_id::HOME_MENU_RESTART));
			add_item(restart, [](pad_button btn) -> page_navigation
			{
				if (btn != pad_button::cross) return page_navigation::stay;

				rsx_log.notice("User selected restart in home menu");

				Emu.CallFromMainThread([]()
				{
					// Make sure we keep the game window opened
					Emu.SetContinuousMode(true);
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
					Emu.GracefulShutdown(true, true);
				});
				return page_navigation::stay;
			});

			apply_layout();
		}
	}
}
