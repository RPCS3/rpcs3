#include "stdafx.h"
#include "overlay_home_menu_savestate.h"
#include "overlay_home_menu_components.h"
#include "Emu/system_config.h"

extern bool boot_last_savestate(bool testing);

namespace rsx
{
	namespace overlays
	{
		home_menu_savestate::home_menu_savestate(s16 x, s16 y, u16 width, u16 height, bool use_separators, home_menu_page* parent)
			: home_menu_page(x, y, width, height, use_separators, parent, get_localized_string(localized_string_id::HOME_MENU_SAVESTATE))
		{
			const bool suspend_mode = g_cfg.savestate.suspend_emu.get();

			std::unique_ptr<overlay_element> save_state = std::make_unique<home_menu_entry>(
				get_localized_string(suspend_mode ? localized_string_id::HOME_MENU_SAVESTATE_AND_EXIT : localized_string_id::HOME_MENU_SAVESTATE_SAVE));

			add_item(save_state, [suspend_mode](pad_button btn) -> page_navigation
			{
				if (btn != pad_button::cross) return page_navigation::stay;
				rsx_log.notice("User selected savestate in home menu");
				Emu.CallFromMainThread([suspend_mode]()
				{
					if (!suspend_mode)
					{
						Emu.after_kill_callback = []() { Emu.Restart(); };

						// Make sure we keep the game window opened
						Emu.SetContinuousMode(true);
					}
					Emu.Kill(false, true);
				});
				return page_navigation::exit;
			});

			if (!suspend_mode && boot_last_savestate(true)) {
				std::unique_ptr<overlay_element> reload_state = std::make_unique<home_menu_entry>(
					get_localized_string(localized_string_id::HOME_MENU_RELOAD_SAVESTATE));

				add_item(reload_state, [](pad_button btn) -> page_navigation
				{
					if (btn != pad_button::cross) return page_navigation::stay;
					rsx_log.notice("User selected reload savestate in home menu");
					Emu.CallFromMainThread([]() { boot_last_savestate(false); });
					return page_navigation::exit;
				});
			}
			apply_layout();
		}
	}
}
