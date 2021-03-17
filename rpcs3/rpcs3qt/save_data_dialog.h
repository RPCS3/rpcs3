#pragma once

#include "util/types.hpp"
#include "Emu/Memory/vm.h"
#include "Emu/Cell/Modules/cellSaveData.h"

#include "util/types.hpp"

class save_data_dialog : public SaveDialogBase
{
public:
	virtual s32 ShowSaveDataList(std::vector<SaveDataEntry>& save_entries, s32 focused, u32 op, vm::ptr<CellSaveDataListSet> listSet, bool enable_overlay) override;
};
