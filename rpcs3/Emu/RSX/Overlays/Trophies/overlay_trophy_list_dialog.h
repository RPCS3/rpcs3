#pragma once

#include "../overlays.h"
#include "../overlay_list_view.hpp"

#include "Loader/TROPUSR.h"

class TROPUSRLoader;

namespace rsx
{
	namespace overlays
	{
		struct trophy_data
		{
			std::unique_ptr<TROPUSRLoader> trop_usr;
			trophy_xml_document trop_config;
			std::unordered_map<int, std::string> trophy_image_paths;
			std::string game_name;
			std::string path;
		};

		struct trophy_list_dialog : public user_interface
		{
		private:
			std::unique_ptr<trophy_data> load_trophies(const std::string& trop_name) const;
			void reload();

			std::unique_ptr<overlay_element> m_dim_background;
			std::unique_ptr<list_view> m_list;
			std::unique_ptr<label> m_description;
			std::unique_ptr<image_button> m_show_hidden_trophies_button;

			animation_color_interpolate fade_animation;

			std::unique_ptr<trophy_data> m_trophy_data;
			atomic_t<bool> m_list_dirty { true };
			bool m_show_hidden_trophies = false;
			bool m_show_hidden_trophies_last = false;

		public:
			trophy_list_dialog();

			void update(u64 timestamp_us) override;
			void on_button_pressed(pad_button button_press, bool is_auto_repeat) override;

			compiled_resource get_compiled() override;

			void show(const std::string& trop_name);
		};
	}
}
