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
			struct skylander_list_entry : horizontal_layout
			{
			public:
				skylander_list_entry(std::string_view name);
			};

			void reload();
			void clear_skylander(u8 sky_slot);
			void load_skylander(u8 sky_slot);

			std::unique_ptr<overlay_element> m_dim_background;
			std::unique_ptr<list_view> m_list;
			std::unique_ptr<label> m_description;
			std::unique_ptr<image_button> m_load_button;
			std::unique_ptr<image_button> m_clear_button;
			std::unique_ptr<image_button> m_create_button;

			animation_color_interpolate fade_animation;

			atomic_t<bool> m_list_dirty{true};

		public:
			skylander_dialog();

			void update(u64 timestamp_us) override;
			void on_button_pressed(pad_button button_press, bool is_auto_repeat) override;

			compiled_resource get_compiled() override;

			void show();
		};

		enum class skylander_file_type
		{
			folder,
			file
		};

		struct skylander_load_dialog : public user_interface
		{
		private:
			struct skylander_file_list_entry : horizontal_layout
			{
			private:
				std::unique_ptr<image_info> icon_data;
				std::string file_name;
				skylander_file_type type;

			public:
				skylander_file_list_entry(skylander_file_type type, const std::string& file_name);
				const std::string& get_file_name() const
				{
					return file_name;
				}
				skylander_file_type get_type() const
				{
					return type;
				}
			};

			void reload();

			u8 slot = 0xff;

			std::unique_ptr<overlay_element> m_dim_background;
			std::unique_ptr<list_view> m_list;
			std::unique_ptr<label> m_description;

			animation_color_interpolate fade_animation;

			atomic_t<bool> m_list_dirty{true};

		public:
			skylander_load_dialog(u8 slot);

			void update(u64 timestamp_us) override;
			void on_button_pressed(pad_button button_press, bool is_auto_repeat) override;

			compiled_resource get_compiled() override;

			void show(std::function<void(s32 status)> on_close);
		};
	} // namespace overlays
} // namespace rsx
