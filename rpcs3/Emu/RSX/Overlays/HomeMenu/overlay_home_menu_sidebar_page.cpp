#include "stdafx.h"
#include "overlay_home_menu_sidebar_page.h"

namespace rsx
{
	namespace overlays
	{
		home_menu_sidebar_page::home_menu_sidebar_page(s16 x, s16 y, u16 width, u16 height, bool use_separators, home_menu_page* parent)
			: home_menu_page(x, y, width, height, use_separators, parent, "")
		{
			m_sidebar = std::make_unique<list_view>(350, overlay::virtual_height, false);
			m_sidebar->set_pos(0, 0);
			m_sidebar->hide_prompt_buttons();
			m_sidebar->back_color = color4f(0.05f, 0.05f, 0.05f, 0.95f);

			m_sliding_animation.duration_sec = 0.5f;
			m_sliding_animation.type = animation_type::ease_in_out_cubic;
		}

		void home_menu_sidebar_page::apply_layout(bool center_vertically)
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

		void home_menu_sidebar_page::add_sidebar_entry(home_menu::fa_icon icon, std::string_view title)
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

		void home_menu_sidebar_page::add_item(home_menu::fa_icon icon, std::string_view title, std::function<page_navigation(pad_button)> callback)
		{
			add_sidebar_entry(icon, title);
			home_menu_page::add_item(home_menu::fa_icon::none, title, callback);
		}

		void home_menu_sidebar_page::add_page(home_menu::fa_icon icon, std::shared_ptr<home_menu_page> page)
		{
			add_sidebar_entry(icon, page->title);
			home_menu_page::add_page(home_menu::fa_icon::none, page);
		}

		void home_menu_sidebar_page::select_entry(s32 entry)
		{
			m_sidebar->select_entry(entry);
			list_view::select_entry(entry);
		}

		void home_menu_sidebar_page::select_next(u16 count)
		{
			m_sidebar->select_next(count);
			list_view::select_next(count);
		}

		void home_menu_sidebar_page::select_previous(u16 count)
		{
			m_sidebar->select_previous(count);
			list_view::select_previous(count);
		}

		void home_menu_sidebar_page::update(u64 timestamp_us)
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

		compiled_resource& home_menu_sidebar_page::get_compiled()
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
