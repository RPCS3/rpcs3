#pragma once

#include "Emu/Cell/Modules/cellMsgDialog.h"

namespace rsx
{
	struct shader_loading_dialog
	{
		std::shared_ptr<MsgDialogBase> dlg;
		atomic_t<int> ref_cnt;

		virtual void create(const std::string& msg, const std::string& title);
		virtual void update_msg(u32 index, const std::string& msg);
		virtual void inc_value(u32 index, u32 value);
		virtual void set_limit(u32 index, u32 limit);
		virtual void refresh();
		virtual void close();
	};
}
