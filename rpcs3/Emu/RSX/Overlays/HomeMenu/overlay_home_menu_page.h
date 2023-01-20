#pragma once

#include "Emu/RSX/Overlays/overlays.h"
#include "Emu/RSX/Overlays/overlay_list_view.hpp"
#include "Emu/RSX/Overlays/HomeMenu/overlay_home_menu_components.h"

namespace rsx
{
	namespace overlays
	{
		enum class page_navigation
		{
			stay,
			back,
			next,
			exit
		};

		struct home_menu_page : public list_view
		{
		public:
			home_menu_page(u16 x, u16 y, u16 width, u16 height, bool use_separators, home_menu_page* parent, const std::string& text);

			void set_current_page(home_menu_page* page);
			home_menu_page* get_current_page(bool include_this);
			page_navigation handle_button_press(pad_button button_press);

			compiled_resource& get_compiled() override;

			bool is_current_page = false;
			home_menu_page* parent = nullptr;
			std::string title;

		protected:
			void add_page(std::shared_ptr<home_menu_page> page);
			void add_item(std::unique_ptr<overlay_element>& element, std::function<page_navigation(pad_button)> callback);
			void apply_layout(bool center_vertically = true);

			std::vector<std::shared_ptr<home_menu_page>> m_pages;

		private:
			std::vector<std::unique_ptr<overlay_element>> m_entries;
			std::vector<std::function<page_navigation(pad_button)>> m_callbacks;
		};
	}
}
