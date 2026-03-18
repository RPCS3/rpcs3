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

atomic_t<bool> g_user_asked_for_fullscreen = false;

namespace rsx
{
	namespace overlays
	{
		home_menu_main_menu::home_menu_main_menu(s16 x, s16 y, u16 width, u16 height, bool use_separators, home_menu_page* parent)
			: home_menu_page(x, y, width, height, use_separators, parent, "")
		{
			is_current_page = true;

			m_message_box = std::make_shared<home_menu_message_box>(x, y, width, height);
			m_message_box->visible = false;

			m_sidebar = std::make_unique<list_view>(350, overlay::virtual_height, false);
			m_sidebar->set_pos(0, 0);
			m_sidebar->hide_prompt_buttons();
			m_sidebar->back_color = color4f(0.05f, 0.05f, 0.05f, 0.95f);

			m_sliding_animation.duration_sec = 0.5f;
			m_sliding_animation.type = animation_type::ease_in_out_cubic;

			add_item(home_menu::fa_icon::back, get_localized_string(localized_string_id::HOME_MENU_RESUME), [](pad_button btn) -> page_navigation
			{
				if (btn != pad_button::cross) return page_navigation::stay;

				rsx_log.notice("User selected resume in home menu");
				return page_navigation::exit;
			});

			add_page(home_menu::fa_icon::settings, std::make_shared<home_menu_settings>(x, y, width, height, use_separators, this));

			if (rsx::overlays::friends_list_dialog::rpcn_configured())
			{
				add_item(home_menu::fa_icon::friends, get_localized_string(localized_string_id::HOME_MENU_FRIENDS), [](pad_button btn) -> page_navigation
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
				add_item(home_menu::fa_icon::trophy, get_localized_string(localized_string_id::HOME_MENU_TROPHIES), [trop_name = std::move(trop_name)](pad_button btn) -> page_navigation
				{
					if (btn != pad_button::cross) return page_navigation::stay;

					rsx_log.notice("User selected trophies in home menu");
					Emu.CallFromMainThread([trop_name]()
					{
						if (auto manager = g_fxo->try_get<rsx::overlays::display_manager>())
						{
							manager->create<rsx::overlays::trophy_list_dialog>()->show(trop_name);
						}
					});
					return page_navigation::stay;
				});
			}

			add_item(home_menu::fa_icon::screenshot, get_localized_string(localized_string_id::HOME_MENU_SCREENSHOT), [](pad_button btn) -> page_navigation
			{
				if (btn != pad_button::cross) return page_navigation::stay;

				rsx_log.notice("User selected screenshot in home menu");
				return page_navigation::exit_for_screenshot;
			});

			add_item(home_menu::fa_icon::video_camera, get_localized_string(localized_string_id::HOME_MENU_RECORDING), [](pad_button btn) -> page_navigation
			{
				if (btn != pad_button::cross) return page_navigation::stay;

				rsx_log.notice("User selected recording in home menu");
				g_user_asked_for_recording = true;
				return page_navigation::exit;
			});

			add_item(home_menu::fa_icon::maximize, get_localized_string(localized_string_id::HOME_MENU_TOGGLE_FULLSCREEN), [](pad_button btn) -> page_navigation
			{
				if (btn != pad_button::cross)
					return page_navigation::stay;

				rsx_log.notice("User selected toggle fullscreen in home menu");
				g_user_asked_for_fullscreen = true;
				return page_navigation::stay; // No need to exit
			});

			add_page(home_menu::fa_icon::floppy, std::make_shared<home_menu_savestate>(x, y, width, height, use_separators, this));

			add_item(home_menu::fa_icon::restart, get_localized_string(localized_string_id::HOME_MENU_RESTART), [](pad_button btn) -> page_navigation
			{
				if (btn != pad_button::cross) return page_navigation::stay;

				rsx_log.notice("User selected restart in home menu");

				Emu.CallFromMainThread([]()
				{
					// Make sure we keep the game window opened
					Emu.SetContinuousMode(true);
					Emu.Restart(true);
				});
				return page_navigation::exit;
			});

			add_item(home_menu::fa_icon::poweroff, get_localized_string(localized_string_id::HOME_MENU_EXIT_GAME), [](pad_button btn) -> page_navigation
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

		void home_menu_main_menu::apply_layout(bool center_vertically)
		{
			home_menu_page::apply_layout(center_vertically);

			if (m_sidebar->get_elements_count() == 0)
			{
				return;
			}

			auto sidebar_items = std::move(m_sidebar->m_items);
			m_sidebar->clear_items();

			u16 combined_height = 0;
			std::for_each(
				sidebar_items.begin(),
				sidebar_items.end(),
				[&](auto& entry)
				{
					combined_height += entry->h + m_sidebar->pack_padding;
					entry->set_pos(0, 0);
				});

			if (combined_height < overlay::virtual_height)
			{
				m_sidebar->advance_pos = (overlay::virtual_height - combined_height) / 2;
			}

			for (auto& entry : sidebar_items)
			{
				m_sidebar->add_entry(entry);
			}
		}

		void home_menu_main_menu::add_sidebar_entry(home_menu::fa_icon icon, std::string_view title)
		{
			auto label_widget = std::make_unique<label>(title.data());
			label_widget->set_size(m_sidebar->w, 60);
			label_widget->set_font("Arial", 16);
			label_widget->back_color.a = 0.f;
			label_widget->set_margin(8, 0);
			label_widget->set_padding(16, 4, 16, 4);
			label_widget->auto_resize();
			label_widget->set_size(label_widget->w, 60);

			if (icon == home_menu::fa_icon::none)
			{
				const u16 packed_width = label_widget->w + 18; // rpad
				if (packed_width > m_sidebar->w)
				{
					m_sidebar->set_size(std::min(packed_width, this->w), m_sidebar->h);
				}
				m_sidebar->add_entry(label_widget);
				return;
			}

			auto icon_info = ensure(home_menu::get_icon(icon));
			auto icon_view = std::make_unique<image_view>();
			icon_view->set_raw_image(icon_info);
			icon_view->set_size(42, 60);
			icon_view->set_margin(8, 0);
			icon_view->set_padding(18, 0, 18, 18);

			const u16 packed_width = icon_view->padding_left + icon_view->w + label_widget->w + 18; // rpad
			if (packed_width > m_sidebar->w)
			{
				m_sidebar->set_size(std::min(packed_width, this->w), m_sidebar->h);
			}

			auto box = std::make_unique<horizontal_layout>();
			box->set_size(0, 16);
			box->set_padding(1);
			box->add_element(icon_view);
			box->add_element(label_widget);

			m_sidebar->add_entry(box);
		}

		void home_menu_main_menu::add_item(home_menu::fa_icon icon, std::string_view title, std::function<page_navigation(pad_button)> callback)
		{
			add_sidebar_entry(icon, title);
			home_menu_page::add_item(home_menu::fa_icon::none, title, callback);
		}

		void home_menu_main_menu::add_page(home_menu::fa_icon icon, std::shared_ptr<home_menu_page> page)
		{
			add_sidebar_entry(icon, page->title);
			home_menu_page::add_page(home_menu::fa_icon::none, page);
		}

		void home_menu_main_menu::select_entry(s32 entry)
		{
			m_sidebar->select_entry(entry);
			list_view::select_entry(entry);
		}

		void home_menu_main_menu::select_next(u16 count)
		{
			m_sidebar->select_next(count);
			list_view::select_next(count);
		}

		void home_menu_main_menu::select_previous(u16 count)
		{
			m_sidebar->select_previous(count);
			list_view::select_previous(count);
		}

		void home_menu_main_menu::update(u64 timestamp_us)
		{
			if (m_animation_timer == 0)
			{
				m_animation_timer = timestamp_us;
				m_sliding_animation.current = { -f32(m_sidebar->x + m_sidebar->w), 0, 0 };
				m_sliding_animation.end = {};
				m_sliding_animation.active = true;
				m_sliding_animation.update(0);
				return;
			}

			if (m_sliding_animation.active)
			{
				m_sliding_animation.update(timestamp_us);
			}
		}

		compiled_resource& home_menu_main_menu::get_compiled()
		{
			m_is_compiled = true;

			if (home_menu_page* page = get_current_page(false))
			{
				return page->get_compiled();
			}

			compiled_resources = m_sidebar->get_compiled();
			m_sliding_animation.apply(compiled_resources);
			return compiled_resources;
		}
	}
}
