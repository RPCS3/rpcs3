#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/system_utils.hpp"
#include "Emu/VFS.h"
#include "Emu/IdManager.h"
#include "Emu/Cell/PPUModule.h"

#include "Emu/RSX/Overlays/overlay_manager.h"
#include "Emu/RSX/Overlays/overlay_user_list_dialog.h"

#include "cellUserInfo.h"

#include "Utilities/StrUtil.h"
#include "cellSysutil.h"

LOG_CHANNEL(cellUserInfo);

struct user_info_manager
{
	atomic_t<bool> enable_overlay{false};
	atomic_t<bool> dialog_opened{false};
};

std::string get_username(const u32 user_id)
{
	std::string username;

	if (const fs::file file{rpcs3::utils::get_hdd0_dir() + fmt::format("home/%08d/localusername", user_id)})
	{
		username = file.to_string();
		username.resize(CELL_USERINFO_USERNAME_SIZE); // TODO: investigate
	}

	return username;
}

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

template<>
void fmt_class_string<cell_user_callback_result>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto error)
	{
		switch (error)
		{
		STR_CASE(CELL_USERINFO_RET_OK);
		STR_CASE(CELL_USERINFO_RET_CANCEL);
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

	const std::string path = vfs::get(fmt::format("/dev_hdd0/home/%08d/", id));

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
	cellUserInfo.warning("cellUserInfoSelectUser_ListType(listType=*0x%x, funcSelect=*0x%x, container=0x%x, userdata=*0x%x)", listType, funcSelect, container, userdata);

	if (!listType || !funcSelect) // TODO: confirm
	{
		return CELL_USERINFO_ERROR_PARAM;
	}

	if (g_fxo->get<user_info_manager>().dialog_opened)
	{
		return CELL_USERINFO_ERROR_BUSY;
	}

	std::vector<u32> user_ids;
	const std::string home_dir = rpcs3::utils::get_hdd0_dir() + "home";

	for (const auto& user_folder : fs::dir(home_dir))
	{
		if (!user_folder.is_directory)
		{
			continue;
		}

		// Is the folder name exactly 8 all-numerical characters long?
		const u32 user_id = rpcs3::utils::check_user(user_folder.name);

		if (user_id == 0)
		{
			continue;
		}

		// Does the localusername file exist?
		if (!fs::is_file(home_dir + "/" + user_folder.name + "/localusername"))
		{
			continue;
		}

		// TODO: maybe also restrict this to CELL_USERINFO_USER_MAX
		if (listType->type != CELL_USERINFO_LISTTYPE_NOCURRENT || user_id != Emu.GetUsrId())
		{
			user_ids.push_back(user_id);
		}
	}

	if (auto manager = g_fxo->try_get<rsx::overlays::display_manager>())
	{
		if (g_fxo->get<user_info_manager>().dialog_opened.exchange(true))
		{
			return CELL_USERINFO_ERROR_BUSY;
		}

		if (s32 ret = sysutil_send_system_cmd(CELL_SYSUTIL_DRAWING_BEGIN, 0); ret < 0)
		{
			g_fxo->get<user_info_manager>().dialog_opened = false;
			return CELL_USERINFO_ERROR_BUSY;
		}

		const std::string title = listType->title.get_ptr();
		const u32 focused = listType->focus;

		cellUserInfo.warning("cellUserInfoSelectUser_ListType: opening user_list_dialog with: title='%s', focused=%d", title, focused);

		const bool enable_overlay = g_fxo->get<user_info_manager>().enable_overlay;
		const error_code result = manager->create<rsx::overlays::user_list_dialog>()->show(title, focused, user_ids, enable_overlay, [funcSelect, userdata](s32 status)
		{
			s32 callback_result = CELL_USERINFO_RET_CANCEL;
			u32 selected_user_id = 0;
			std::string selected_username;

			if (status >= 0)
			{
				callback_result = CELL_USERINFO_RET_OK;
				selected_user_id = static_cast<u32>(status);
				selected_username = get_username(selected_user_id);
			}

			cellUserInfo.warning("cellUserInfoSelectUser_ListType: callback_result=%s, selected_user_id=%d, selected_username='%s'", callback_result, selected_user_id, selected_username);

			g_fxo->get<user_info_manager>().dialog_opened = false;

			sysutil_send_system_cmd(CELL_SYSUTIL_DRAWING_END, 0);

			sysutil_register_cb([=](ppu_thread& ppu) -> s32
			{
				vm::var<CellUserInfoUserStat> selectUser;
				if (status >= 0)
				{
					selectUser->id = selected_user_id;
					strcpy_trunc(selectUser->name, selected_username);
				}
				funcSelect(ppu, callback_result, selectUser, userdata);
				return CELL_OK;
			});
		});

		return result;
	}

	cellUserInfo.error("User selection is only possible when the native user interface is enabled in the settings. The currently active user will be selected as a fallback.");

	sysutil_register_cb([=](ppu_thread& ppu) -> s32
	{
		vm::var<CellUserInfoUserStat> selectUser;
		selectUser->id = Emu.GetUsrId();
		strcpy_trunc(selectUser->name, get_username(Emu.GetUsrId()));
		funcSelect(ppu, CELL_USERINFO_RET_OK, selectUser, userdata);
		return CELL_OK;
	});

	return CELL_OK;
}

error_code cellUserInfoSelectUser_SetList(vm::ptr<CellUserInfoListSet> setList, vm::ptr<CellUserInfoFinishCallback> funcSelect, u32 container, vm::ptr<void> userdata)
{
	cellUserInfo.warning("cellUserInfoSelectUser_SetList(setList=*0x%x, funcSelect=*0x%x, container=0x%x, userdata=*0x%x)", setList, funcSelect, container, userdata);

	if (!setList || !funcSelect) // TODO: confirm
	{
		return CELL_USERINFO_ERROR_PARAM;
	}

	if (g_fxo->get<user_info_manager>().dialog_opened)
	{
		return CELL_USERINFO_ERROR_BUSY;
	}

	std::vector<u32> user_ids;

	for (usz i = 0; i < CELL_USERINFO_USER_MAX && i < setList->fixedListNum; i++)
	{
		if (const u32 id = setList->fixedList->userId[i])
		{
			user_ids.push_back(id);
		}
	}

	if (user_ids.empty())
	{
		// TODO: Confirm. Also check if this is possible in cellUserInfoSelectUser_ListType.
		cellUserInfo.error("cellUserInfoSelectUser_SetList: callback_result=%s", CELL_USERINFO_ERROR_NOUSER);

		sysutil_register_cb([=](ppu_thread& ppu) -> s32
		{
			vm::var<CellUserInfoUserStat> selectUser;
			funcSelect(ppu, CELL_USERINFO_ERROR_NOUSER, selectUser, userdata);
			return CELL_OK;
		});

		return CELL_OK;
	}

	// TODO: does this function return an error if any (user_id > 0 && not_found) ?

	if (auto manager = g_fxo->try_get<rsx::overlays::display_manager>())
	{
		if (g_fxo->get<user_info_manager>().dialog_opened.exchange(true))
		{
			return CELL_USERINFO_ERROR_BUSY;
		}

		if (s32 ret = sysutil_send_system_cmd(CELL_SYSUTIL_DRAWING_BEGIN, 0); ret < 0)
		{
			g_fxo->get<user_info_manager>().dialog_opened = false;
			return CELL_USERINFO_ERROR_BUSY;
		}

		const std::string title = setList->title.get_ptr();
		const u32 focused = setList->focus;

		cellUserInfo.warning("cellUserInfoSelectUser_SetList: opening user_list_dialog with: title='%s', focused=%d", title, focused);

		const bool enable_overlay = g_fxo->get<user_info_manager>().enable_overlay;
		const error_code result = manager->create<rsx::overlays::user_list_dialog>()->show(title, focused, user_ids, enable_overlay, [funcSelect, userdata](s32 status)
		{
			s32 callback_result = CELL_USERINFO_RET_CANCEL;
			u32 selected_user_id = 0;
			std::string selected_username;

			if (status >= 0)
			{
				callback_result = CELL_USERINFO_RET_OK;
				selected_user_id = static_cast<u32>(status);
				selected_username = get_username(selected_user_id);
			}

			cellUserInfo.warning("cellUserInfoSelectUser_SetList: callback_result=%s, selected_user_id=%d, selected_username='%s'", callback_result, selected_user_id, selected_username);

			g_fxo->get<user_info_manager>().dialog_opened = false;

			sysutil_send_system_cmd(CELL_SYSUTIL_DRAWING_END, 0);

			sysutil_register_cb([=](ppu_thread& ppu) -> s32
			{
				vm::var<CellUserInfoUserStat> selectUser;
				if (status >= 0)
				{
					selectUser->id = selected_user_id;
					strcpy_trunc(selectUser->name, selected_username);
				}
				funcSelect(ppu, callback_result, selectUser, userdata);
				return CELL_OK;
			});
		});

		return result;
	}

	cellUserInfo.error("User selection is only possible when the native user interface is enabled in the settings. The currently active user will be selected as a fallback.");

	sysutil_register_cb([=](ppu_thread& ppu) -> s32
	{
		vm::var<CellUserInfoUserStat> selectUser;
		selectUser->id = Emu.GetUsrId();
		strcpy_trunc(selectUser->name, get_username(Emu.GetUsrId()));
		funcSelect(ppu, CELL_USERINFO_RET_OK, selectUser, userdata);
		return CELL_OK;
	});

	return CELL_OK;
}

void cellUserInfoEnableOverlay(s32 enable)
{
	cellUserInfo.notice("cellUserInfoEnableOverlay(enable=%d)", enable);
	auto& manager = g_fxo->get<user_info_manager>();
	manager.enable_overlay = enable != 0;
}

error_code cellUserInfoGetList(vm::ptr<u32> listNum, vm::ptr<CellUserInfoUserList> listBuf, vm::ptr<u32> currentUserId)
{
	cellUserInfo.warning("cellUserInfoGetList(listNum=*0x%x, listBuf=*0x%x, currentUserId=*0x%x)", listNum, listBuf, currentUserId);

	// If only listNum is NULL, an error will be returned
	if (!listNum)
	{
		if (listBuf || !currentUserId)
		{
			return CELL_USERINFO_ERROR_PARAM;
		}
	}

	const std::string home_dir = rpcs3::utils::get_hdd0_dir() + "home";
	std::vector<u32> user_ids;

	for (const auto& user_folder : fs::dir(home_dir))
	{
		if (!user_folder.is_directory)
		{
			continue;
		}

		// Is the folder name exactly 8 all-numerical characters long?
		const u32 user_id = rpcs3::utils::check_user(user_folder.name);

		if (user_id == 0)
		{
			continue;
		}

		// Does the localusername file exist?
		if (!fs::is_file(home_dir + "/" + user_folder.name + "/localusername"))
		{
			continue;
		}

		if (user_ids.size() < CELL_USERINFO_USER_MAX)
		{
			user_ids.push_back(user_id);
		}
		else
		{
			cellUserInfo.warning("cellUserInfoGetList: Cannot add user %s. Too many users.", user_folder.name);
		}
	}

	if (listNum)
	{
		*listNum = static_cast<u32>(user_ids.size());
	}

	if (listBuf)
	{
		for (usz i = 0; i < CELL_USERINFO_USER_MAX; i++)
		{
			if (i < user_ids.size())
			{
				listBuf->userId[i] = user_ids[i];
			}
			else
			{
				listBuf->userId[i] = 0;
			}
		}
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
