#include "save_data_dialog.h"
#include "save_data_list_dialog.h"

#include "Emu/System.h"
#include "Emu/IdManager.h"
#include "Emu/Io/interception.h"
#include "../Emu/RSX/Overlays/overlay_manager.h"
#include "Emu/RSX/Overlays/overlay_save_dialog.h"
#include "Emu/Cell/Modules/cellSysutil.h"

#include "util/logs.hpp"

LOG_CHANNEL(cellSaveData);

s32 save_data_dialog::ShowSaveDataList(const std::string& base_dir, std::vector<SaveDataEntry>& save_entries, s32 focused, u32 op, vm::ptr<CellSaveDataListSet> listSet, bool enable_overlay)
{
	cellSaveData.notice("ShowSaveDataList(save_entries=%d, focused=%d, op=0x%x, listSet=*0x%x, enable_overlay=%d)", save_entries.size(), focused, op, listSet, enable_overlay);

	// TODO: Implement proper error checking in savedata_op?
	const bool use_end = sysutil_send_system_cmd(CELL_SYSUTIL_DRAWING_BEGIN, 0) >= 0;

	if (!use_end)
	{
		cellSaveData.error("ShowSaveDataList(): Not able to notify DRAWING_BEGIN callback because one has already been sent!");
	}

	// TODO: Install native shell as an Emu callback
	if (auto manager = g_fxo->try_get<rsx::overlays::display_manager>())
	{
		cellSaveData.notice("ShowSaveDataList: Showing native UI dialog");

		const s32 result = manager->create<rsx::overlays::save_dialog>()->show(base_dir, save_entries, focused, op, listSet, enable_overlay);
		if (result != rsx::overlays::user_interface::selection_code::error)
		{
			cellSaveData.notice("ShowSaveDataList: Native UI dialog returned with selection %d", result);
			if (use_end) sysutil_send_system_cmd(CELL_SYSUTIL_DRAWING_END, 0);
			return result;
		}

		cellSaveData.error("ShowSaveDataList: Native UI dialog returned error");
	}

	if (!Emu.HasGui())
	{
		cellSaveData.notice("ShowSaveDataList(): Aborting: Emulation has no GUI attached");
		if (use_end) sysutil_send_system_cmd(CELL_SYSUTIL_DRAWING_END, 0);
		return -2;
	}

	// Fall back to front-end GUI
	cellSaveData.notice("ShowSaveDataList(): Using fallback GUI");
	atomic_t<s32> selection = 0;

	input::SetIntercepted(true);

	Emu.BlockingCallFromMainThread([&]()
	{
		save_data_list_dialog sdid(save_entries, focused, op, listSet);
		sdid.exec();
		selection = sdid.GetSelection();
	});

	input::SetIntercepted(false);

	if (use_end) sysutil_send_system_cmd(CELL_SYSUTIL_DRAWING_END, 0);

	return selection.load();
}
