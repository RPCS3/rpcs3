#include "stdafx.h"
#include "Emu/Memory/Memory.h"

#include "save_data_dialog.h"
#include "save_data_utility.h"

s32 save_data_dialog::ShowSaveDataList(std::vector<SaveDataEntry>& save_entries, s32 focused, vm::ptr<CellSaveDataListSet> listSet)
{
	save_data_list_dialog sdid(save_entries, false);
	sdid.exec();
	return sdid.m_selectedEntry;
}
