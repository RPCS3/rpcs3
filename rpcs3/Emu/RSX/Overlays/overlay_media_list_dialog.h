#pragma once

#include "overlays.h"
#include "overlay_list_view.hpp"
#include "Emu/Cell/ErrorCodes.h"
#include "util/media_utils.h"

namespace rsx
{
	namespace overlays
	{
		struct media_list_dialog : public user_interface
		{
		public:
			enum class media_type
			{
				invalid,   // For internal use only
				directory, // For internal use only
				audio,
				video,
				photo,
			};

			struct media_entry
			{
				media_type type = media_type::invalid;
				std::string name;
				std::string path;
				utils::media_info info;
				u32 index = 0;

				media_entry* parent = nullptr;
				std::vector<media_entry> children;
			};

			media_list_dialog();

			void on_button_pressed(pad_button button_press, bool is_auto_repeat) override;

			compiled_resource get_compiled() override;

			s32 show(media_entry* root, media_entry& result, const std::string& title, u32 focused, bool enable_overlay);

		private:
			void reload(const std::string& title, u32 focused);

			struct media_list_entry : horizontal_layout
			{
			public:
				media_list_entry(const media_entry& entry);

			private:
				std::unique_ptr<image_info> icon_data;
			};

			media_entry* m_media = nullptr;

			std::unique_ptr<overlay_element> m_dim_background;
			std::unique_ptr<list_view> m_list;
			std::unique_ptr<label> m_description;
			std::unique_ptr<label> m_no_media_text;
		};

		error_code show_media_list_dialog(media_list_dialog::media_type type, const std::string& path, const std::string& title, std::function<void(s32 status, utils::media_info info)> on_finished);
	}
}
