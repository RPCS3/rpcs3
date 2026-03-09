#include "stdafx.h"
#include "overlay_home_menu_components.h"

namespace rsx
{
	namespace overlays
	{
		home_menu_entry::home_menu_entry(const std::string& text, u16 width)
		{
			std::unique_ptr<overlay_element> text_stack = std::make_unique<vertical_layout>();
			std::unique_ptr<overlay_element> padding    = std::make_unique<spacer>();
			std::unique_ptr<overlay_element> title      = std::make_unique<label>(text);

			padding->set_size(1, 1);
			title->set_size(width, menu_entry_height);
			title->set_font("Arial", 16);
			title->set_wrap_text(true);
			title->align_text(text_align::center);

			// Make back color transparent for text
			title->back_color.a = 0.f;

			static_cast<vertical_layout*>(text_stack.get())->pack_padding = 5;
			static_cast<vertical_layout*>(text_stack.get())->add_element(padding);
			static_cast<vertical_layout*>(text_stack.get())->add_element(title);

			add_element(text_stack);
		}

		home_menu_checkbox::home_menu_checkbox(cfg::_bool* setting, const std::string& text) : home_menu_setting(setting, text)
		{}

		void home_menu_checkbox::set_size(u16 w, u16 h)
		{
			set_reserved_width(w / 2 + menu_entry_margin);
			home_menu_setting::set_size(w, h);

			auto box = std::make_unique<box_layout>();
			m_background = box->add_element();
			m_checkbox = box->add_element();

			m_background->set_size(menu_checkbox_size, menu_checkbox_size);
			m_checkbox->set_size(m_background->w - 2, m_background->h - 2);
			m_checkbox->set_pos(1, 1);

			box->set_pos(0, 16);
			add_element(box);
		}

		compiled_resource& home_menu_checkbox::get_compiled()
		{
			update_value();

			if (!is_compiled())
			{
				m_background->back_color = { 1.f };
				m_checkbox->back_color = { 0.3f };
				m_checkbox->back_color.a = 1.f;
				m_checkbox->set_visible(!m_last_value);

				compiled_resources = horizontal_layout::get_compiled();
				compiled_resources.add(m_background->get_compiled());
				compiled_resources.add(m_checkbox->get_compiled());
			}

			return compiled_resources;
		}
	}
}
