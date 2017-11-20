#include "save_data_dialog.h"
#include "save_data_list_dialog.h"

s32 save_data_dialog::ShowSaveDataList(std::vector<SaveDataEntry>& save_entries, s32 focused, u32 op, vm::ptr<CellSaveDataListSet> listSet)
{
	save_data_list_dialog sdid(save_entries, focused, op, listSet);
	sdid.exec();
	return sdid.GetSelection();
}
