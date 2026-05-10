#pragma once

#include "overlays.h"
#include "overlay_list_view.hpp"
#include "Emu/Cell/Modules/cellSaveData.h"

namespace rsx
{
	namespace overlays
	{
		struct save_dialog : public user_interface
		{
		private:
			struct save_dialog_entry : horizontal_layout
			{
			public:
				save_dialog_entry(const std::string& text1, const std::string& text2, const std::string& text3, u8 resource_id, const std::vector<u8>& icon_buf, const std::string& video_path);
				void set_selected(bool selected) override;

			private:
				overlay_element* m_image = nullptr;
			};

			std::unique_ptr<overlay_element> m_dim_background;
			std::unique_ptr<list_view> m_list;
			std::unique_ptr<label> m_description;
			std::unique_ptr<label> m_time_thingy;
			std::unique_ptr<label> m_no_saves_text;

			bool m_no_saves = false;

			animation_color_interpolate fade_animation;

		public:
			save_dialog();

			void update(u64 timestamp_us) override;
			void on_button_pressed(pad_button button_press, bool is_auto_repeat) override;

			compiled_resource get_compiled() override;

			s32 show(const std::string& base_dir, std::vector<SaveDataEntry>& save_entries, u32 focused, u32 op, vm::ptr<CellSaveDataListSet> listSet, bool enable_overlay);
		};
	}
}
