#include "save_data_dialog.h"
#include "save_data_list_dialog.h"

#include <Emu/IdManager.h>
#include <Emu/RSX/Overlays/overlays.h>

s32 save_data_dialog::ShowSaveDataList(std::vector<SaveDataEntry>& save_entries, s32 focused, u32 op, vm::ptr<CellSaveDataListSet> listSet)
{
	//TODO: Install native shell as an Emu callback
	if (auto manager = fxm::get<rsx::overlays::display_manager>())
	{
		auto result = manager->create<rsx::overlays::save_dialog>()->show(save_entries, op, listSet);
		if (result != rsx::overlays::user_interface::selection_code::error)
			return result;
	}

	//Fall back to front-end GUI
	atomic_t<bool> dlg_result(false);
	atomic_t<s32> selection;

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

	return selection.load();
}
