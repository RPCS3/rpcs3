#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/VFS.h"
#include "Emu/Cell/PPUModule.h"

#include "cellUserInfo.h"

#include "Utilities/StrUtil.h"
#include "cellSysutil.h"

LOG_CHANNEL(cellUserInfo);

template<>
void fmt_class_string<CellUserInfoError>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto error)
	{
		switch (error)
		{
		STR_CASE(CELL_USERINFO_ERROR_BUSY);
		STR_CASE(CELL_USERINFO_ERROR_INTERNAL);
		STR_CASE(CELL_USERINFO_ERROR_PARAM);
		STR_CASE(CELL_USERINFO_ERROR_NOUSER);
		}

		return unknown;
	});
}

error_code cellUserInfoGetStat(u32 id, vm::ptr<CellUserInfoUserStat> stat)
{
	cellUserInfo.warning("cellUserInfoGetStat(id=%d, stat=*0x%x)", id, stat);

	if (id > CELL_SYSUTIL_USERID_MAX)
	{
		// ****** sysutil userinfo parameter error : 1 ******
		return {CELL_USERINFO_ERROR_PARAM, "1"};
	}

	if (id == CELL_SYSUTIL_USERID_CURRENT)
	{
		// We want the int value, not the string.
		id = Emu.GetUsrId();
	}

	const std::string& path = vfs::get(fmt::format("/dev_hdd0/home/%08d/", id));

	if (!fs::is_dir(path))
	{
		cellUserInfo.error("cellUserInfoGetStat(): CELL_USERINFO_ERROR_NOUSER. User %d doesn't exist. Did you delete the user folder?", id);
		return CELL_USERINFO_ERROR_NOUSER;
	}

	const fs::file f(path + "localusername");

	if (!f)
	{
		cellUserInfo.error("cellUserInfoGetStat(): CELL_USERINFO_ERROR_INTERNAL. Username for user %08u doesn't exist. Did you delete the username file?", id);
		return CELL_USERINFO_ERROR_INTERNAL;
	}

	if (stat)
	{
		stat->id = id;
		strcpy_trunc(stat->name, f.to_string());
	}

	return CELL_OK;
}

error_code cellUserInfoSelectUser_ListType(vm::ptr<CellUserInfoTypeSet> listType, vm::ptr<CellUserInfoFinishCallback> funcSelect, u32 container, vm::ptr<void> userdata)
{
	cellUserInfo.todo("cellUserInfoSelectUser_ListType(listType=*0x%x, funcSelect=*0x%x, container=0x%x, userdata=*0x%x)", listType, funcSelect, container, userdata);

	sysutil_register_cb([=](ppu_thread& ppu) -> s32
	{
		vm::var<CellUserInfoUserStat> selectUser;
		funcSelect(ppu, CELL_OK, selectUser, userdata);
		return CELL_OK;
	});

	return CELL_OK;
}

error_code cellUserInfoSelectUser_SetList(vm::ptr<CellUserInfoListSet> setList, vm::ptr<CellUserInfoFinishCallback> funcSelect, u32 container, vm::ptr<void> userdata)
{
	cellUserInfo.todo("cellUserInfoSelectUser_SetList(setList=*0x%x, funcSelect=*0x%x, container=0x%x, userdata=*0x%x)", setList, funcSelect, container, userdata);

	sysutil_register_cb([=](ppu_thread& ppu) -> s32
	{
		vm::var<CellUserInfoUserStat> selectUser;
		funcSelect(ppu, CELL_OK, selectUser, userdata);
		return CELL_OK;
	});

	return CELL_OK;
}

void cellUserInfoEnableOverlay(s32 enable)
{
	cellUserInfo.todo("cellUserInfoEnableOverlay(enable=%d)", enable);
}

error_code cellUserInfoGetList(vm::ptr<u32> listNum, vm::ptr<CellUserInfoUserList> listBuf, vm::ptr<u32> currentUserId)
{
	cellUserInfo.todo("cellUserInfoGetList(listNum=*0x%x, listBuf=*0x%x, currentUserId=*0x%x)", listNum, listBuf, currentUserId);

	// If only listNum is NULL, an error will be returned
	if (!listNum)
	{
		if (listBuf || !currentUserId)
		{
			return CELL_USERINFO_ERROR_PARAM;
		}
	}

	if (listNum)
	{
		*listNum = 1;
	}

	if (listBuf)
	{
		std::memset(listBuf.get_ptr(), 0, listBuf.size());

		// We report only one user, so it must be the current user
		listBuf->userId[0] = Emu.GetUsrId();
	}

	if (currentUserId)
	{
		// We want the int value, not the string.
		*currentUserId = Emu.GetUsrId();
	}

	return CELL_OK;
}

DECLARE(ppu_module_manager::cellUserInfo)("cellUserInfo", []()
{
	REG_FUNC(cellUserInfo, cellUserInfoGetStat);
	REG_FUNC(cellUserInfo, cellUserInfoSelectUser_ListType);
	REG_FUNC(cellUserInfo, cellUserInfoSelectUser_SetList);
	REG_FUNC(cellUserInfo, cellUserInfoEnableOverlay);
	REG_FUNC(cellUserInfo, cellUserInfoGetList);
});
