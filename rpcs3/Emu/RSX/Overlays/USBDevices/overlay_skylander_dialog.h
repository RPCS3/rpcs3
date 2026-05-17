#pragma once

#include "../overlays.h"
#include "../overlay_list_view.hpp"

namespace rsx
{
	namespace overlays
	{
		struct skylander_dialog : public user_interface
		{
		private:
			std::unique_ptr<overlay_element> m_dim_background;
			std::unique_ptr<list_view> m_list;
			std::unique_ptr<label> m_description;
			std::unique_ptr<image_button> m_show_hidden_trophies_button;

			animation_color_interpolate fade_animation;

			atomic_t<bool> m_list_dirty{true};
			bool m_show_hidden_trophies = false;
			bool m_show_hidden_trophies_last = false;

		public:
			skylander_dialog();

			void update(u64 timestamp_us) override;
			void on_button_pressed(pad_button button_press, bool is_auto_repeat) override;

			compiled_resource get_compiled() override;

			void show();
		};
	} // namespace overlays
} // namespace rsx
