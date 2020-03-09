#include "stdafx.h"
#include "shader_loading_dialog_native.h"
#include "../overlay_message_dialog.h"

namespace rsx
{
	shader_loading_dialog_native::shader_loading_dialog_native(GSRender* ptr)
		: owner(ptr)
	{
	}

	void shader_loading_dialog_native::create(const std::string& msg, const std::string&/* title*/)
	{
		MsgDialogType type = {};
		type.disable_cancel = true;
		type.progress_bar_count = 2;

		dlg = g_fxo->get<rsx::overlays::display_manager>()->create<rsx::overlays::message_dialog>(!!g_cfg.video.shader_preloading_dialog.use_custom_background);
		dlg->progress_bar_set_taskbar_index(-1);
		dlg->show(false, msg, type, [](s32 status)
		{
			if (status != CELL_OK)
				Emu.Stop();
		});
	}

	void shader_loading_dialog_native::update_msg(u32 index, const std::string& msg)
	{
		dlg->progress_bar_set_message(index, msg);
		owner->flip({});
	}

	void shader_loading_dialog_native::inc_value(u32 index, u32 value)
	{
		dlg->progress_bar_increment(index, static_cast<f32>(value));
		owner->flip({});
	}

	void shader_loading_dialog_native::set_limit(u32 index, u32 limit)
	{
		dlg->progress_bar_set_limit(index, limit);
		owner->flip({});
	}

	void shader_loading_dialog_native::refresh()
	{
		dlg->refresh();
	}

	void shader_loading_dialog_native::close()
	{
		dlg->return_code = CELL_OK;
		dlg->close(false, false);
	}
}
