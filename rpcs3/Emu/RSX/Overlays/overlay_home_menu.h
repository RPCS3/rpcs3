#pragma once

#include "overlays.h"
#include "overlay_list_view.hpp"
#include "Emu/Cell/ErrorCodes.h"

namespace rsx
{
	namespace overlays
	{
		struct home_menu_dialog : public user_interface
		{
		private:
			struct home_menu_entry : horizontal_layout
			{
			public:
				home_menu_entry(const std::string& text);

				static constexpr u16 height = 40;
			};

			std::vector<std::unique_ptr<overlay_element>> m_entries;
			std::vector<std::function<bool()>> m_callbacks;
			std::unique_ptr<overlay_element> m_dim_background;
			std::unique_ptr<list_view> m_list;
			std::unique_ptr<label> m_description;

			animation_color_interpolate fade_animation;

		public:
			home_menu_dialog();

			void update() override;
			void on_button_pressed(pad_button button_press) override;

			compiled_resource get_compiled() override;

			void add_item(const std::string& text, std::function<bool()> callback);
			error_code show(std::function<void(s32 status)> on_close);
		};
	}
}
