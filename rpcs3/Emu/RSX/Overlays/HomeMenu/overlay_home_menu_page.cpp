#include "stdafx.h"
#include "overlay_home_menu_page.h"
#include "Emu/System.h"

namespace rsx
{
	namespace overlays
	{
		home_menu_page::home_menu_page(u16 x, u16 y, u16 width, u16 height, bool use_separators, home_menu_page* parent, const std::string& title)
			: list_view(width, height, use_separators)
			, title(title)
			, parent(parent)
		{
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

		page_navigation home_menu_page::handle_button_press(pad_button button_press)
		{
			if (home_menu_page* page = get_current_page(false))
			{
				return page->handle_button_press(button_press);
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
						Emu.GetCallbacks().play_sound(fs::get_config_dir() + "sounds/snd_decide.wav");
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

			Emu.GetCallbacks().play_sound(fs::get_config_dir() + "sounds/snd_cursor.wav");
			return page_navigation::stay;
		}

		compiled_resource& home_menu_page::get_compiled()
		{
			if (home_menu_page* page = get_current_page(false))
			{
				return page->get_compiled();
			}

			return list_view::get_compiled();
		}
	}
}
