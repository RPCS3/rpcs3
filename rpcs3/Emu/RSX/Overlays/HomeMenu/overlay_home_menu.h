#pragma once

#include "Emu/RSX/Overlays/overlays.h"
#include "Emu/Cell/ErrorCodes.h"
#include "overlay_home_menu_main_menu.h"

namespace rsx
{
	namespace overlays
	{
		struct home_menu_dialog : public user_interface
		{
		public:
			explicit home_menu_dialog();

			void update() override;
			void on_button_pressed(pad_button button_press) override;

			compiled_resource get_compiled() override;

			error_code show(std::function<void(s32 status)> on_close);

		private:
			std::unique_ptr<overlay_element> m_dim_background;
			std::unique_ptr<home_menu_main_menu> m_main_menu;
			std::unique_ptr<label> m_description;
			std::unique_ptr<label> m_time_display;

			animation_color_interpolate fade_animation;
		};
	}
}
