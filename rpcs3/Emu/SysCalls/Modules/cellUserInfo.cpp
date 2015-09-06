#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/SysCalls/Modules.h"

#include "Emu/FS/VFS.h"
#include "Emu/FS/vfsFileBase.h"
#include "cellUserInfo.h"

extern Module cellUserInfo;

s32 cellUserInfoGetStat(u32 id, vm::ptr<CellUserInfoUserStat> stat)
{
	cellUserInfo.Warning("cellUserInfoGetStat(id=%d, stat=*0x%x)", id, stat);

	if (id > CELL_USERINFO_USER_MAX)
	{
		return CELL_USERINFO_ERROR_NOUSER;
	}

	char path[256];
	sprintf(path, "/dev_hdd0/home/%08d", id);

	if (!Emu.GetVFS().ExistsDir(path))
	{
		return CELL_USERINFO_ERROR_NOUSER;
	}

	sprintf(path, "/dev_hdd0/home/%08d/localusername", id);
	vfsStream* stream = Emu.GetVFS().OpenFile(path, fom::read);

	if (!stream || !(stream->IsOpened()))
	{
		return CELL_USERINFO_ERROR_INTERNAL;
	}

	char name [CELL_USERINFO_USERNAME_SIZE];
	memset(name, 0, CELL_USERINFO_USERNAME_SIZE);
	stream->Read(name, CELL_USERINFO_USERNAME_SIZE);
	stream->Close();
	delete stream;

	stat->id = id;
	memcpy(stat->name, name, CELL_USERINFO_USERNAME_SIZE);

	return CELL_OK;
}

s32 cellUserInfoSelectUser_SetList(PPUThread& ppu, vm::ptr<CellUserInfoListSet> setList, vm::ptr<CellUserInfoFinishCallback> funcSelect, u32 container, vm::ptr<u32> userdata)
{
	cellUserInfo.Todo("cellUserInfoSelectUser_SetList(setList=*0x%x, funcSelect=0x%x, container=%d, userdata=*0x%x)", setList, funcSelect, container, userdata);

	// TODO: Proper implementation, we currently return a cancellation of the dialog
	funcSelect(ppu, CELL_USERINFO_RET_CANCEL, vm::ptr<CellUserInfoUserStat>::make(0), userdata);

	return CELL_OK;
}

s32 cellUserInfoSelectUser_ListType(PPUThread& ppu, vm::ptr<CellUserInfoTypeSet> setType, vm::ptr<CellUserInfoFinishCallback> funcSelect, u32 container, vm::ptr<u32> userdata)
{
	cellUserInfo.Todo("cellUserInfoSelectUser_ListType(setType=*0x%x, funcSelect=0x%x, container=%d, userdata=*0x%x)", setType, funcSelect, container, userdata);

	// TODO: Proper implementation, we currently return a cancellation of the dialog
	funcSelect(ppu, CELL_USERINFO_RET_CANCEL, vm::ptr<CellUserInfoUserStat>::make(0), userdata);

	return CELL_OK;
}

s32 cellUserInfoEnableOverlay(s32 enable)
{
	cellUserInfo.Todo("cellUserInfoEnableOverlay(enable=%d)", enable);
	return CELL_OK;
}

s32 cellUserInfoGetList(vm::ptr<u32> listNum, vm::ptr<CellUserInfoUserList> listBuf, vm::ptr<u32> currentUserId)
{
	cellUserInfo.Warning("cellUserInfoGetList(listNum=*0x%x, listBuf=*0x%x, currentUserId=*0x%x)", listNum, listBuf, currentUserId);

	// If only listNum is NULL, an error will be returned
	if (listBuf && !listNum)
	{
		return CELL_USERINFO_ERROR_PARAM;
	}

	if (listNum)
	{
		*listNum = 1;
	}

	if (listBuf)
	{
		listBuf->userId[0] = 1;
	}

	if (currentUserId)
	{
		*currentUserId = 1;
	}
	
	return CELL_OK;
}

Module cellUserInfo("cellUserInfo", []()
{
	REG_FUNC(cellUserInfo, cellUserInfoGetStat);
	REG_FUNC(cellUserInfo, cellUserInfoSelectUser_ListType);
	REG_FUNC(cellUserInfo, cellUserInfoSelectUser_SetList);
	REG_FUNC(cellUserInfo, cellUserInfoEnableOverlay);
	REG_FUNC(cellUserInfo, cellUserInfoGetList);
});
