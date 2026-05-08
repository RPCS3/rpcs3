#pragma once

#include "Emu/RSX/Overlays/overlay_list_view.hpp"
#include "Emu/RSX/Overlays/HomeMenu/overlay_home_icons.h"
#include "Emu/RSX/Overlays/HomeMenu/overlay_home_menu_components.h"
#include "Emu/RSX/Overlays/HomeMenu/overlay_home_menu_message_box.h"

namespace rsx
{
	namespace overlays
	{
		struct home_menu_popup
		{
			std::function<compiled_resource&()> get_compiled;
			std::function<page_navigation(pad_button)> input_hook;

			void dismiss()
			{
				get_compiled = {};
				input_hook = {};
			}

			operator bool() const { return !!get_compiled; }
		};

		struct home_menu_page : public list_view
		{
		public:
			home_menu_page(s16 x, s16 y, u16 width, u16 height, bool use_separators, home_menu_page* parent, const std::string& text);

			void set_current_page(home_menu_page* page);
			home_menu_page* get_current_page(bool include_this);
			virtual page_navigation handle_button_press(pad_button button_press, bool is_auto_repeat, u64 auto_repeat_interval_ms);

			void translate(s16 _x, s16 _y) override;
			void set_size(u16 _w, u16 _h) override;
			compiled_resource& get_compiled() override;

			void on_activate();
			void on_deactivate();

			virtual void update(u64 /*timestamp_us*/) {}
			virtual bool show_reset_button() const { return false; }

			bool is_current_page = false;
			home_menu_page* parent = nullptr;
			std::string title;

			std::shared_ptr<home_menu_message_box> m_message_box;
			std::shared_ptr<bool> m_config_changed;

			home_menu_popup m_popup;

		protected:
			virtual void add_page(home_menu::fa_icon icon, std::shared_ptr<home_menu_page> page);
			virtual void add_item(home_menu::fa_icon icon, std::string_view, std::function<page_navigation(pad_button)> callback);
			virtual void add_item(std::unique_ptr<overlay_element>& element, std::function<page_navigation(pad_button)> callback);
			virtual void apply_layout(bool center_vertically = false);
			void show_dialog(const std::string& text, std::function<void()> on_accept = nullptr, std::function<void()> on_cancel = nullptr);

			std::vector<std::shared_ptr<home_menu_page>> m_pages;

		private:
			image_button m_reset_btn;
			image_button m_save_btn;
			image_button m_discard_btn;
			std::vector<std::unique_ptr<overlay_element>> m_entries;
			std::vector<std::function<page_navigation(pad_button)>> m_callbacks;
		};
	}
}
