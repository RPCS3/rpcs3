#pragma once

#include "overlays.h"
#include "Emu/Cell/Modules/cellMsgDialog.h"

namespace rsx
{
	namespace overlays
	{
		class message_dialog : public user_interface
		{
			label text_display;
			image_button btn_ok;
			image_button btn_cancel;

			overlay_element bottom_bar, background;
			image_view background_poster;
			progress_bar progress_1, progress_2;
			u8 num_progress_bars = 0;
			s32 taskbar_index = 0;
			s32 taskbar_limit = 0;

			bool interactive = false;
			bool ok_only = false;
			bool cancel_only = false;

			std::unique_ptr<image_info> background_image;

		public:
			message_dialog(bool use_custom_background = false);

			compiled_resource get_compiled() override;

			void on_button_pressed(pad_button button_press) override;

			error_code show(bool is_blocking, const std::string& text, const MsgDialogType& type, std::function<void(s32 status)> on_close);

			u32 progress_bar_count();
			void progress_bar_set_taskbar_index(s32 index);
			error_code progress_bar_set_message(u32 index, const std::string& msg);
			error_code progress_bar_increment(u32 index, f32 value);
			error_code progress_bar_reset(u32 index);
			error_code progress_bar_set_limit(u32 index, u32 limit);
		};
	}
}
