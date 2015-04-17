#pragma once

#include "Emu/SysCalls/Modules/cellSaveData.h"

class SaveDataDialogFrame : public SaveDataDialogInstance
{
public:
	virtual s32 ShowSaveDataList(std::vector<SaveDataEntry>& save_entries, s32 focused, vm::ptr<CellSaveDataListSet> listSet) override;
};
