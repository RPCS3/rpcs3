#include "stdafx.h"
#include "overlay_home_menu_voice_chat.h"
#include "overlay_home_menu_components.h"
#include "Emu/system_config.h"
#include "Emu/Cell/Modules/cellSysutilAvc2.h"

namespace rsx
{
	namespace overlays
	{
		home_menu_voice_chat::home_menu_voice_chat(s16 x, s16 y, u16 width, u16 height, bool use_separators, home_menu_page* parent)
			: home_menu_settings_page(x, y, width, height, use_separators, parent, get_localized_string(localized_string_id::HOME_MENU_VOICE_CHAT))
		{
			add_checkbox(&g_cfg.net.voice_chat_enabled, localized_string_id::HOME_MENU_VOICE_CHAT_ENABLE);
			add_signed_slider(&g_cfg.audio.voice_chat_mic_volume, localized_string_id::HOME_MENU_VOICE_CHAT_MIC_VOLUME, " %", 1);

			const std::vector<avc2_voice_chat_member> members = cellSysutilAvc2GetVoiceChatMembers();

			if (members.empty())
			{
				std::unique_ptr<overlay_element> elem = std::make_unique<home_menu_entry>(home_menu::fa_icon::none, get_localized_string(localized_string_id::HOME_MENU_VOICE_CHAT_NO_PLAYERS), width, text_align::left);
				add_item(elem, [](pad_button) -> page_navigation { return page_navigation::stay; });
			}

			for (const avc2_voice_chat_member& member : members)
			{
				auto muted = std::make_shared<bool>(member.muted);

				std::unique_ptr<overlay_element> elem = std::make_unique<home_menu_checkbox>([muted]() { return *muted; },
					get_localized_string(localized_string_id::HOME_MENU_VOICE_CHAT_MUTE_PLAYER, "%s", member.npid));
				elem->set_size(width, menu_entry_height);

				add_item(elem, [this, muted, npid = member.npid](pad_button btn) -> page_navigation
				{
					if (btn != pad_button::cross) return page_navigation::stay;

					*muted = !*muted;
					rsx_log.notice("User %s voice chat member '%s' in home menu", *muted ? "muted" : "unmuted", npid);
					cellSysutilAvc2SetVoiceChatMemberMuting(npid, *muted);
					refresh();
					return page_navigation::stay;
				});
			}

			apply_layout();
		}
	}
}
