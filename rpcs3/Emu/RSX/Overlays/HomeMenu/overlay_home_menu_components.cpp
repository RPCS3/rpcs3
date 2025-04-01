#include "stdafx.h"
#include "overlay_home_menu_components.h"

namespace rsx
{
	namespace overlays
	{
		home_menu_entry::home_menu_entry(const std::string& text)
		{
			std::unique_ptr<overlay_element> text_stack = std::make_unique<vertical_layout>();
			std::unique_ptr<overlay_element> padding    = std::make_unique<spacer>();
			std::unique_ptr<overlay_element> title      = std::make_unique<label>(text);

			padding->set_size(1, 1);
			title->set_size(overlay::virtual_width - 2 * menu_entry_margin, menu_entry_height);
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
		{
			m_background.set_size(menu_entry_margin, menu_entry_margin);
			m_background.set_pos(overlay::virtual_width / 2 + menu_entry_margin, 0);

			m_checkbox.set_size(m_background.w - 2, m_background.h - 2);
			m_checkbox.set_pos(m_background.x, m_background.y);
		}

		compiled_resource& home_menu_checkbox::get_compiled()
		{
			update_value();

			if (!is_compiled())
			{
				const f32 col = m_last_value ? 1.0f : 0.3f;
				const f32 bkg = m_last_value ? 0.3f : 1.0f;
				m_background.back_color.r = bkg;
				m_background.back_color.g = bkg;
				m_background.back_color.b = bkg;
				m_checkbox.back_color.r = col;
				m_checkbox.back_color.g = col;
				m_checkbox.back_color.b = col;

				m_background.set_pos(m_background.x, y + (h - m_background.h) / 2);
				m_checkbox.set_pos(m_background.x + 1, m_background.y + 1);

				compiled_resources = horizontal_layout::get_compiled();
				compiled_resources.add(m_background.get_compiled());
				compiled_resources.add(m_checkbox.get_compiled());
			}

			return compiled_resources;
		}
	}
}
