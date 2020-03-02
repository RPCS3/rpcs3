#include "stdafx.h"
#include "shader_loading_dialog.h"
#include "Emu/System.h"

namespace rsx
{
	void shader_loading_dialog::create(const std::string& msg, const std::string& title)
	{
		dlg = Emu.GetCallbacks().get_msg_dialog();
		if (dlg)
		{
			dlg->type.se_normal = true;
			dlg->type.bg_invisible = true;
			dlg->type.progress_bar_count = 2;
			dlg->ProgressBarSetTaskbarIndex(-1); // -1 to combine all progressbars in the taskbar progress
			dlg->on_close = [](s32 status) { Emu.CallAfter([]() { Emu.Stop(); }); };

			ref_cnt++;

			Emu.CallAfter([&]()
			{
				dlg->Create(msg, title);
				ref_cnt--;
			});
		}

		while (ref_cnt.load() && !Emu.IsStopped())
		{
			_mm_pause();
		}
	}

	void shader_loading_dialog::update_msg(u32 index, const std::string& msg)
	{
		if (!dlg)
		{
			return;
		}

		ref_cnt++;

		Emu.CallAfter([&, index, msg]()
		{
			dlg->ProgressBarSetMsg(index, msg);
			ref_cnt--;
		});
	}

	void shader_loading_dialog::inc_value(u32 index, u32 value)
	{
		if (!dlg)
		{
			return;
		}

		ref_cnt++;

		Emu.CallAfter([&, index, value]()
		{
			dlg->ProgressBarInc(index, value);
			ref_cnt--;
		});
	}

	void shader_loading_dialog::set_limit(u32 index, u32 limit)
	{
		if (!dlg)
		{
			return;
		}

		ref_cnt++;

		Emu.CallAfter([&, index, limit]()
		{
			dlg->ProgressBarSetLimit(index, limit);
			ref_cnt--;
		});
	}

	void shader_loading_dialog::refresh()
	{
	}

	void shader_loading_dialog::close()
	{
		while (ref_cnt.load() && !Emu.IsStopped())
		{
			_mm_pause();
		}
	}
}
