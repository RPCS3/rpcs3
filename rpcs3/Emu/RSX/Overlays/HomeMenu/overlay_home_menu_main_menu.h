#pragma once

#include "overlay_home_menu_page.h"
#include "../overlay_animation.h"

namespace rsx
{
	namespace overlays
	{
		struct home_menu_main_menu : public home_menu_page
		{
			home_menu_main_menu(s16 x, s16 y, u16 width, u16 height, bool use_separators, home_menu_page* parent);

			void select_entry(s32 entry) override;
			void select_next(u16 count = 1) override;
			void select_previous(u16 count = 1) override;
			compiled_resource& get_compiled() override;

			void update(u64 timestamp_us) override;

		private:
			void apply_layout(bool center_vertically = false) override;

			using home_menu_page::add_item;
			void add_item(home_menu::fa_icon icon, std::string_view title, std::function<page_navigation(pad_button)> callback) override;
			void add_page(home_menu::fa_icon icon, std::shared_ptr<home_menu_page> page) override;

			void add_sidebar_entry(home_menu::fa_icon icon, std::string_view title);

			u64 m_animation_timer = 0;
			animation_translate m_sliding_animation;
			std::unique_ptr<list_view> m_sidebar; // Render proxy
		};
	}
}
