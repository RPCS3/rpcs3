#include "save_data_dialog.h"
#include "save_data_list_dialog.h"

#include <Emu/System.h>
#include <Emu/IdManager.h>
#include <Emu/RSX/Overlays/overlay_save_dialog.h>

#include "Input/pad_thread.h"

s32 save_data_dialog::ShowSaveDataList(std::vector<SaveDataEntry>& save_entries, s32 focused, u32 op, vm::ptr<CellSaveDataListSet> listSet)
{
	// TODO: Install native shell as an Emu callback
	if (auto manager = g_fxo->get<rsx::overlays::display_manager>())
	{
		auto result = manager->create<rsx::overlays::save_dialog>()->show(save_entries, focused, op, listSet);
		if (result != rsx::overlays::user_interface::selection_code::error)
			return result;
	}

	if (!Emu.HasGui())
	{
		return -2;
	}

	// Fall back to front-end GUI
	atomic_t<bool> dlg_result(false);
	atomic_t<s32> selection;

	pad::SetIntercepted(true);

	Emu.CallAfter([&]()
	{
		save_data_list_dialog sdid(save_entries, focused, op, listSet);
		sdid.exec();
		selection = sdid.GetSelection();
		dlg_result = true;
	});

	while (!dlg_result)
	{
		thread_ctrl::wait_for(1000);
	}

	pad::SetIntercepted(false);

	return selection.load();
}
