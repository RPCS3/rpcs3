#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/SC_FUNC.h"

#include "cellUserInfo.h"

void cellUserInfo_init();
Module cellUserInfo(0x0032, cellUserInfo_init);

int cellUserInfoGetStat(u32 id, mem_ptr_t<CellUserInfoUserStat> stat)
{
	cellUserInfo.Warning("cellUserInfoGetStat(id=%d, stat_addr=0x%x)", id, stat.GetAddr());

	if (!stat.IsGood())
		return CELL_USERINFO_ERROR_PARAM;
	if (id > CELL_USERINFO_USER_MAX)
		return CELL_USERINFO_ERROR_NOUSER;

	char path [256];
	sprintf(path, "/dev_hdd0/home/%08d", id);
	if (!Emu.GetVFS().ExistsDir(path))
		return CELL_USERINFO_ERROR_NOUSER;

	sprintf(path, "/dev_hdd0/home/%08d/localusername", id);
	vfsStream* stream = Emu.GetVFS().OpenFile(path, vfsRead);
	if (!stream || !(stream->IsOpened()))
		return CELL_USERINFO_ERROR_INTERNAL;

	char name [CELL_USERINFO_USERNAME_SIZE];
	memset(name, 0, CELL_USERINFO_USERNAME_SIZE);
	stream->Read(name, CELL_USERINFO_USERNAME_SIZE);
	stream->Close();
	delete stream;

	stat->id = id;
	memcpy(stat->name, name, CELL_USERINFO_USERNAME_SIZE);
	return CELL_OK;
}

int cellUserInfoSelectUser_ListType()
{
	UNIMPLEMENTED_FUNC(cellUserInfo);
	return CELL_OK;
}

int cellUserInfoSelectUser_SetList()
{
	UNIMPLEMENTED_FUNC(cellUserInfo);
	return CELL_OK;
}

int cellUserInfoEnableOverlay()
{
	UNIMPLEMENTED_FUNC(cellUserInfo);
	return CELL_OK;
}

int cellUserInfoGetList(mem32_t listNum, mem_ptr_t<CellUserInfoUserList> listBuf, mem32_t currentUserId)
{
	cellUserInfo.Warning("cellUserInfoGetList(listNum_addr=0x%x, listBuf_addr=0x%x, currentUserId_addr=0x%x)",
		listNum.GetAddr(), listBuf.GetAddr(), currentUserId.GetAddr());

	listNum = 1;
	listBuf->userId[0] = 1;
	currentUserId = 1;
	return CELL_OK;
}

void cellUserInfo_init()
{
	cellUserInfo.AddFunc(0x2b761140, cellUserInfoGetStat);
	cellUserInfo.AddFunc(0x3097cc1c, cellUserInfoSelectUser_ListType);
	cellUserInfo.AddFunc(0x55123a25, cellUserInfoSelectUser_SetList);
	cellUserInfo.AddFunc(0xb3516536, cellUserInfoEnableOverlay);
	cellUserInfo.AddFunc(0xc55e338b, cellUserInfoGetList);
}
