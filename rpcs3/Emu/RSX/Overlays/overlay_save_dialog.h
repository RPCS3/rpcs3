#pragma once

#include "overlays.h"
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
			private:
				std::unique_ptr<image_info> icon_data;

			public:
				save_dialog_entry(const std::string& text1, const std::string& text2, const std::string& text3, u8 resource_id, const std::vector<u8>& icon_buf);
			};

			std::unique_ptr<overlay_element> m_dim_background;
			std::unique_ptr<list_view> m_list;
			std::unique_ptr<label> m_description;
			std::unique_ptr<label> m_time_thingy;
			std::unique_ptr<label> m_no_saves_text;

			bool m_no_saves = false;

		public:
			save_dialog();

			void update() override;
			void on_button_pressed(pad_button button_press) override;

			compiled_resource get_compiled() override;

			s32 show(std::vector<SaveDataEntry>& save_entries, u32 focused, u32 op, vm::ptr<CellSaveDataListSet> listSet);
		};
	}
}
