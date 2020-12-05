#pragma once

#include "stdafx.h"
#include "Emu/Memory/vm.h"
#include "Emu/Cell/Modules/cellSaveData.h"

class save_data_dialog : public SaveDialogBase
{
public:
	virtual s32 ShowSaveDataList(std::vector<SaveDataEntry>& save_entries, s32 focused, u32 op, vm::ptr<CellSaveDataListSet> listSet) override;
};
