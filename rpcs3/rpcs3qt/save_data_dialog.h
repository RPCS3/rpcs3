#ifndef SAVEDATADIALOG_H
#define SAVEDATADIALOG_H

#include "Emu/Cell/Modules/cellSaveData.h"

class save_data_dialog : public SaveDialogBase
{
public:
	virtual s32 ShowSaveDataList(std::vector<SaveDataEntry>& save_entries, s32 focused, vm::ptr<CellSaveDataListSet> listSet) override;
};


#endif // !SAVEDATADIALOG_H
