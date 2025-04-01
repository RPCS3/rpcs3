#include "stdafx.h"
#include "overlay_home_menu_page.h"
#include "Emu/System.h"
#include "Emu/system_config.h"

namespace rsx
{
	namespace overlays
	{
		home_menu_page::home_menu_page(s16 x, s16 y, u16 width, u16 height, bool use_separators, home_menu_page* parent, const std::string& title)
			: list_view(width, height, use_separators)
			, parent(parent)
			, title(title)
			, m_save_btn(120, 30)
			, m_discard_btn(120, 30)
		{
			if (parent)
			{
				m_message_box = parent->m_message_box;
				m_config_changed = parent->m_config_changed;
			}

			m_save_btn.set_image_resource(resource_config::standard_image_resource::square);
			m_discard_btn.set_image_resource(resource_config::standard_image_resource::triangle);

			m_save_btn.set_pos(width - 2 * (30 + 120), height + 20);
			m_discard_btn.set_pos(width - (30 + 120), height + 20);

			m_save_btn.set_text(localized_string_id::HOME_MENU_SETTINGS_SAVE_BUTTON);
			m_discard_btn.set_text(localized_string_id::HOME_MENU_SETTINGS_DISCARD_BUTTON);

			m_save_btn.set_font("Arial", 16);
			m_discard_btn.set_font("Arial", 16);

			set_pos(x, y);
		}

		void home_menu_page::set_current_page(home_menu_page* page)
		{
			if (page)
			{
				is_current_page = false;
				page->is_current_page = true;
				rsx_log.notice("Home menu: changing current page from '%s' to '%s'", title, page->title);
			}
		}

		home_menu_page* home_menu_page::get_current_page(bool include_this)
		{
			if (is_current_page)
			{
				if (include_this)
				{
					return this;
				}
			}
			else
			{
				for (auto& page : m_pages)
				{
					if (page)
					{
						if (home_menu_page* p = page->get_current_page(true))
						{
							return p;
						}
					}
				}
			}

			return nullptr;
		}

		void home_menu_page::add_page(std::shared_ptr<home_menu_page> page)
		{
			ensure(page);
			std::unique_ptr<overlay_element> elem = std::make_unique<home_menu_entry>(page->title);
			m_pages.push_back(page);

			add_item(elem, [this, page](pad_button btn) -> page_navigation
			{
				if (btn != pad_button::cross) return page_navigation::stay;

				rsx_log.notice("User selected '%s' in '%s'", page->title, title);
				set_current_page(page.get());
				return page_navigation::next;
			});
		}

		void home_menu_page::add_item(std::unique_ptr<overlay_element>& element, std::function<page_navigation(pad_button)> callback)
		{
			m_callbacks.push_back(std::move(callback));
			m_entries.push_back(std::move(element));
		}

		void home_menu_page::apply_layout(bool center_vertically)
		{
			// Center vertically if necessary
			if (center_vertically)
			{
				usz total_height = 0;

				for (auto& entry : m_entries)
				{
					total_height += entry->h;
				}

				if (total_height < h)
				{
					advance_pos = (h - ::narrow<u16>(total_height)) / 2;
				}
			}

			for (auto& entry : m_entries)
			{
				add_entry(entry);
			}
		}

		void home_menu_page::show_dialog(const std::string& text, std::function<void()> on_accept, std::function<void()> on_cancel)
		{
			if (m_message_box && !m_message_box->visible)
			{
				rsx_log.notice("home_menu_page::show_dialog: page='%s', text='%s'", title, text);
				m_message_box->show(text, std::move(on_accept), std::move(on_cancel));
				refresh();
			}
		}

		page_navigation home_menu_page::handle_button_press(pad_button button_press, bool is_auto_repeat, u64 auto_repeat_interval_ms)
		{
			if (m_message_box && m_message_box->visible)
			{
				const page_navigation navigation = m_message_box->handle_button_press(button_press);
				if (navigation != page_navigation::stay)
				{
					m_message_box->hide();
					refresh();
				}
				return navigation;
			}

			if (home_menu_page* page = get_current_page(false))
			{
				return page->handle_button_press(button_press, is_auto_repeat, auto_repeat_interval_ms);
			}

			switch (button_press)
			{
			case pad_button::dpad_left:
			case pad_button::dpad_right:
			case pad_button::ls_left:
			case pad_button::ls_right:
			case pad_button::cross:
			{
				if (const usz index = static_cast<usz>(get_selected_index()); index < m_callbacks.size())
				{
					if (const std::function<page_navigation(pad_button)>& func = ::at32(m_callbacks, index))
					{
						// Play a sound unless this is a fast auto repeat which would induce a nasty noise
						if (!is_auto_repeat || auto_repeat_interval_ms >= user_interface::m_auto_repeat_ms_interval_default)
						{
							Emu.GetCallbacks().play_sound(fs::get_config_dir() + "sounds/snd_decide.wav");
						}
						return func(button_press);
					}
				}
				break;
			}
			case pad_button::circle:
			{
				Emu.GetCallbacks().play_sound(fs::get_config_dir() + "sounds/snd_cancel.wav");
				if (parent)
				{
					set_current_page(parent);
					return page_navigation::back;
				}
				return page_navigation::exit;
			}
			case pad_button::triangle:
			{
				if (m_config_changed && *m_config_changed)
				{
					show_dialog(get_localized_string(localized_string_id::HOME_MENU_SETTINGS_DISCARD), [this]()
					{
						rsx_log.notice("home_menu_page: discarding settings...");

						if (m_config_changed && *m_config_changed)
						{
							g_cfg.from_string(g_backup_cfg.to_string());
							Emu.GetCallbacks().update_emu_settings();
							*m_config_changed = false;
						}
					});
				}
				break;
			}
			case pad_button::square:
			{
				if (m_config_changed && *m_config_changed)
				{
					show_dialog(get_localized_string(localized_string_id::HOME_MENU_SETTINGS_SAVE), [this]()
					{
						rsx_log.notice("home_menu_page: saving settings...");
						Emu.GetCallbacks().save_emu_settings();

						if (m_config_changed)
						{
							*m_config_changed = false;
						}
					});
				}
				break;
			}
			case pad_button::dpad_up:
			case pad_button::ls_up:
			{
				select_previous();
				break;
			}
			case pad_button::dpad_down:
			case pad_button::ls_down:
			{
				select_next();
				break;
			}
			case pad_button::L1:
			{
				select_previous(10);
				break;
			}
			case pad_button::R1:
			{
				select_next(10);
				break;
			}
			default:
			{
				rsx_log.trace("[ui] Button %d pressed", static_cast<u8>(button_press));
				break;
			}
			}

			// Play a sound unless this is a fast auto repeat which would induce a nasty noise
			if (!is_auto_repeat || auto_repeat_interval_ms >= user_interface::m_auto_repeat_ms_interval_default)
			{
				Emu.GetCallbacks().play_sound(fs::get_config_dir() + "sounds/snd_cursor.wav");
			}
			return page_navigation::stay;
		}

		void home_menu_page::translate(s16 _x, s16 _y)
		{
			list_view::translate(_x, _y);
			m_save_btn.translate(_x, _y);
			m_discard_btn.translate(_x, _y);
		}

		compiled_resource& home_menu_page::get_compiled()
		{
			if (!is_compiled() || (m_message_box && !m_message_box->is_compiled()))
			{
				m_is_compiled = false;

				if (home_menu_page* page = get_current_page(false))
				{
					compiled_resources = page->get_compiled();
				}
				else
				{
					compiled_resources = list_view::get_compiled();

					if (m_message_box && m_message_box->visible)
					{
						compiled_resources.add(m_message_box->get_compiled());
					}
					else if (m_config_changed && *m_config_changed)
					{
						compiled_resources.add(m_save_btn.get_compiled());
						compiled_resources.add(m_discard_btn.get_compiled());
					}
				}

				m_is_compiled = true;
			}

			return compiled_resources;
		}
	}
}
