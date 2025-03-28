#include "stdafx.h"
#include "overlay_list_view.hpp"
#include "Emu/system_config.h"

namespace rsx
{
	namespace overlays
	{
		list_view::list_view(u16 width, u16 height, bool use_separators, bool can_deny)
			: m_use_separators(use_separators)
		{
			w = width;
			h = height;

			m_scroll_indicator_top    = std::make_unique<image_view>(width, 5);
			m_scroll_indicator_bottom = std::make_unique<image_view>(width, 5);
			m_accept_btn              = std::make_unique<image_button>(120, 20);
			m_cancel_btn              = std::make_unique<image_button>(120, 20);
			m_highlight_box           = std::make_unique<overlay_element>(width, 0);

			m_scroll_indicator_top->set_size(width, 40);
			m_scroll_indicator_bottom->set_size(width, 40);
			m_accept_btn->set_size(120, 30);
			m_cancel_btn->set_size(120, 30);

			m_scroll_indicator_top->set_image_resource(resource_config::standard_image_resource::fade_top);
			m_scroll_indicator_bottom->set_image_resource(resource_config::standard_image_resource::fade_bottom);

			if (g_cfg.sys.enter_button_assignment == enter_button_assign::circle)
			{
				m_accept_btn->set_image_resource(resource_config::standard_image_resource::circle);
				m_cancel_btn->set_image_resource(resource_config::standard_image_resource::cross);
			}
			else
			{
				m_accept_btn->set_image_resource(resource_config::standard_image_resource::cross);
				m_cancel_btn->set_image_resource(resource_config::standard_image_resource::circle);
			}

			m_scroll_indicator_bottom->set_pos(0, height - 40);
			m_accept_btn->set_pos(30, height + 20);

			if (can_deny)
			{
				m_deny_btn = std::make_unique<image_button>(120, 20);
				m_deny_btn->set_size(120, 30);
				m_deny_btn->set_image_resource(resource_config::standard_image_resource::triangle);
				m_deny_btn->set_pos(180, height + 20);
				m_deny_btn->set_text(localized_string_id::RSX_OVERLAYS_LIST_DENY);
				m_deny_btn->set_font("Arial", 16);

				m_cancel_btn->set_pos(330, height + 20);
			}
			else
			{
				m_cancel_btn->set_pos(180, height + 20);
			}

			m_accept_btn->set_text(localized_string_id::RSX_OVERLAYS_LIST_SELECT);
			m_cancel_btn->set_text(localized_string_id::RSX_OVERLAYS_LIST_CANCEL);

			m_accept_btn->set_font("Arial", 16);
			m_cancel_btn->set_font("Arial", 16);

			auto_resize = false;
			back_color  = {0.15f, 0.15f, 0.15f, 0.8f};

			m_highlight_box->back_color             = {.5f, .5f, .8f, 0.2f};
			m_highlight_box->pulse_effect_enabled   = true;
			m_scroll_indicator_top->fore_color.a    = 0.f;
			m_scroll_indicator_bottom->fore_color.a = 0.f;
		}

		const overlay_element* list_view::get_selected_entry() const
		{
			if (m_selected_entry < 0)
			{
				return nullptr; // Ideally unreachable but it should still be possible to recover by user interaction.
			}

			const usz current_index = static_cast<usz>(m_selected_entry) * (m_use_separators ? 2 : 1);

			if (current_index >= m_items.size())
			{
				return nullptr; // Ideally unreachable but it should still be possible to recover by user interaction.
			}

			return m_items[current_index].get();
		}

		void list_view::update_selection()
		{
			const overlay_element* current_element = get_selected_entry();

			for (auto& item : m_items)
			{
				if (item)
				{
					item->set_selected(item.get() == current_element);
				}
			}

			if (!current_element)
			{
				return; // Ideally unreachable but it should still be possible to recover by user interaction.
			}

			// Calculate bounds
			const auto min_y = current_element->y - y;
			const auto max_y = current_element->y + current_element->h + pack_padding + 2 - y;

			if (min_y < scroll_offset_value)
			{
				scroll_offset_value = min_y;
			}
			else if (max_y > (h + scroll_offset_value))
			{
				scroll_offset_value = max_y - h - 2;
			}

			if ((scroll_offset_value + h + 2) >= m_elements_height)
				m_scroll_indicator_bottom->fore_color.a = 0.f;
			else
				m_scroll_indicator_bottom->fore_color.a = 0.5f;

			if (scroll_offset_value == 0)
				m_scroll_indicator_top->fore_color.a = 0.f;
			else
				m_scroll_indicator_top->fore_color.a = 0.5f;

			m_highlight_box->set_pos(current_element->x, current_element->y);
			m_highlight_box->h = current_element->h + pack_padding;
			m_highlight_box->y -= scroll_offset_value;

			m_highlight_box->refresh();
			m_scroll_indicator_top->refresh();
			m_scroll_indicator_bottom->refresh();
			refresh();
		}

		void list_view::select_entry(s32 entry)
		{
			const s32 max_entry = m_elements_count - 1;

			// Reset the pulse slightly below 1 rising on each user interaction
			m_highlight_box->set_sinus_offset(1.6f);

			if (m_selected_entry != entry)
			{
				m_selected_entry = std::max(0, std::min(entry, max_entry));
				update_selection();
			}
			else
			{
				refresh();
			}
		}

		void list_view::select_next(u16 count)
		{
			select_entry(m_selected_entry + count);
		}

		void list_view::select_previous(u16 count)
		{
			select_entry(m_selected_entry - count);
		}

		void list_view::add_entry(std::unique_ptr<overlay_element>& entry)
		{
			// Add entry view
			add_element(entry);
			m_elements_count++;

			// Add separator
			if (m_use_separators)
			{
				auto separator        = std::make_unique<overlay_element>();
				separator->back_color = fore_color;
				separator->w          = w;
				separator->h          = 2;
				add_element(separator);
			}

			if (m_selected_entry < 0)
				m_selected_entry = 0;

			m_elements_height = advance_pos;
			update_selection();
		}

		int list_view::get_selected_index() const
		{
			return m_selected_entry;
		}

		void list_view::set_cancel_only(bool cancel_only)
		{
			if (cancel_only)
				m_cancel_btn->set_pos(x + 30, y + h + 20);
			else if (m_deny_btn)
				m_cancel_btn->set_pos(x + 330, y + h + 20);
			else
				m_cancel_btn->set_pos(x + 180, y + h + 20);

			m_cancel_only = cancel_only;
			m_is_compiled = false;
		}

		bool list_view::get_cancel_only() const
		{
			return m_cancel_only;
		}

		void list_view::translate(s16 _x, s16 _y)
		{
			layout_container::translate(_x, _y);
			m_scroll_indicator_top->translate(_x, _y);
			m_scroll_indicator_bottom->translate(_x, _y);
			m_accept_btn->translate(_x, _y);
			m_cancel_btn->translate(_x, _y);

			if (m_deny_btn)
			{
				m_deny_btn->translate(_x, _y);
			}
		}

		compiled_resource& list_view::get_compiled()
		{
			if (!is_compiled())
			{
				auto& compiled = vertical_layout::get_compiled();
				compiled.add(m_highlight_box->get_compiled());
				compiled.add(m_scroll_indicator_top->get_compiled());
				compiled.add(m_scroll_indicator_bottom->get_compiled());
				compiled.add(m_cancel_btn->get_compiled());

				if (!m_cancel_only)
				{
					compiled.add(m_accept_btn->get_compiled());

					if (m_deny_btn)
					{
						compiled.add(m_deny_btn->get_compiled());
					}
				}

				compiled_resources = compiled;
			}

			return compiled_resources;
		}
	} // namespace overlays
} // namespace rsx
