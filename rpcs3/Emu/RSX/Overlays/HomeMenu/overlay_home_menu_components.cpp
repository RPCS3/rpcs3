#include "stdafx.h"
#include "overlay_home_menu_components.h"

namespace rsx
{
	namespace overlays
	{
		home_menu_entry::home_menu_entry(home_menu::fa_icon icon, const std::string& text, u16 width, text_align alignment)
		{
			auto text_stack = std::make_unique<vertical_layout>();
			auto padding    = std::make_unique<spacer>();
			auto title      = std::make_unique<label>(text);

			padding->set_size(1, 1);
			title->set_size(width, menu_entry_height);
			title->set_font("Arial", 14);
			title->set_wrap_text(true);
			title->align_text(alignment);

			// Make back color transparent for text
			title->back_color.a = 0.f;

			text_stack->pack_padding = 8;
			text_stack->add_element(padding);
			text_stack->add_element(title);

			if (icon == home_menu::fa_icon::none)
			{
				add_element(text_stack);
				return;
			}

			auto icon_info = ensure(home_menu::get_icon(icon));
			auto icon_view = std::make_unique<image_view>();
			icon_view->set_raw_image(icon_info);
			icon_view->set_size(32, text_stack->h);

			if (text_stack->h > 24)
			{
				const u16 pad = (text_stack->h - 24) / 2;
				icon_view->set_padding(0, 8, pad, pad);
			}

			const auto image_size_with_padding = icon_view->w + 16;
			if (alignment == text_align::center)
			{
				if (width > image_size_with_padding)
				{
					auto text = text_stack->m_items.back().get();
					static_cast<label*>(text)->auto_resize(false, width - image_size_with_padding);

					const auto content_w = text->w + image_size_with_padding;
					const auto offset = (std::max<u16>(width, content_w) - content_w) / 2;

					// For autosized containers, just set the initial size, let the packing grow it automatically
					this->set_size(offset, menu_entry_height);
				}
			}
			else if (alignment == text_align::right)
			{
				auto text = text_stack->m_items.back().get();
				static_cast<label*>(text)->auto_resize(false, width - image_size_with_padding);

				if (const u16 combined_w = (text->w + image_size_with_padding); combined_w < width)
				{
					// Insert a spacer
					std::unique_ptr<overlay_element> spacer_element = std::make_unique<spacer>();
					spacer_element->set_size(combined_w, icon_view->h);
					add_element(spacer_element);
				}
			}
			else
			{
				// Left align
				add_spacer(menu_entry_height);
			}

			add_element(icon_view);
			add_element(text_stack);
		}

		home_menu_checkbox::home_menu_checkbox(cfg::_bool* setting, const std::string& text)
			: home_menu_setting(setting, text)
		{}

		void home_menu_checkbox::set_size(u16 w, u16 h)
		{
			set_reserved_width(menu_checkbox_size * 2 + menu_entry_margin);
			home_menu_setting::set_size(w, h);

			auto checkbox_ = std::make_unique<switchbox>();
			checkbox_->set_size(menu_checkbox_size, menu_checkbox_size);
			checkbox_->set_pos(0, 16);
			m_checkbox = add_element(checkbox_);
		}

		compiled_resource& home_menu_checkbox::get_compiled()
		{
			update_value();

			if (is_compiled())
			{
				return compiled_resources;
			}

			m_checkbox->set_checked(m_last_value);
			return horizontal_layout::get_compiled();
		}
	}
}
