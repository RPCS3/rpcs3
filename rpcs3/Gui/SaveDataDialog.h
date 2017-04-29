#pragma once

#include "Emu/Cell/Modules/cellSaveData.h"

class SaveDialogFrame : public SaveDialogBase
{
public:
	virtual s32 ShowSaveDataList(std::vector<SaveDataEntry>& save_entries, s32 focused, vm::ptr<CellSaveDataListSet> listSet) override;
};
