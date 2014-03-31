#pragma once
#include "Dialog.h"
#include "Emu/SysCalls/Modules/cellSysutil_SaveData.h"

class SaveDataList : public Dialog
{
public:
	void Load(const std::vector<SaveDataListEntry>& entries);
};