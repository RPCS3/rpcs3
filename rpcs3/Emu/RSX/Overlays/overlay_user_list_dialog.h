#pragma once

#include "overlays.h"
#include "overlay_list_view.hpp"
#include "Emu/Cell/ErrorCodes.h"

namespace rsx
{
	namespace overlays
	{
		struct user_list_dialog : public user_interface
		{
		private:
			struct user_list_entry : horizontal_layout
			{
			private:
				std::unique_ptr<image_info> icon_data;

			public:
				user_list_entry(const std::string& username, const std::string& user_id, const std::string& avatar_path);
			};

			std::vector<u32> m_entry_ids;
			std::unique_ptr<overlay_element> m_dim_background;
			std::unique_ptr<list_view> m_list;
			std::unique_ptr<label> m_description;

			animation_color_interpolate fade_animation;

		public:
			user_list_dialog();

			void update(u64 timestamp_us) override;
			void on_button_pressed(pad_button button_press, bool is_auto_repeat) override;

			compiled_resource get_compiled() override;

			error_code show(const std::string& title, u32 focused, const std::vector<u32>& user_ids, bool enable_overlay, std::function<void(s32 status)> on_close);
		};
	}
}
