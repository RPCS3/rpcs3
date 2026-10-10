#pragma once

#include "Emu/RSX/Overlays/overlays.h"
#include "Emu/RSX/Overlays/overlay_controls.h"
#include "Emu/RSX/Overlays/overlay_video.h"
#include "overlay_big_picture_game_info.h"

namespace rsx
{
	namespace overlays
	{
		// A full-screen panel shown on top of the game grid, describing the highlighted game with a "Start" prompt.
		struct big_picture_game_details
		{
		public:
			big_picture_game_details(s16 x, s16 y, u16 width, u16 height);

			enum class result
			{
				stay,
				back,
				start
			};

			void show(const big_picture_game_info& info, const image_info* icon_data);
			void hide();
			bool is_visible() const { return m_visible; }

			result handle_button_press(pad_button button_press);

			compiled_resource& get_compiled();

		private:
			s16 m_x = 0;
			s16 m_y = 0;
			u16 m_w = 0;
			u16 m_h = 0;

			bool m_visible = false;
			compiled_resource m_compiled;

			overlay_element m_dim_background{};
			image_view m_pic_background{};
			std::unique_ptr<image_info> m_pic_data;
			image_view m_icon{};
			std::unique_ptr<video_view> m_video;
			label m_title{};
			label m_info_text{};
			image_button m_start_btn{ 120, 30 };
			image_button m_back_hint{ 120, 30 };
		};
	}
}
