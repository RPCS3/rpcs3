#include "stdafx.h"
#include "Utilities/Log.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/SysCalls/Modules.h"
#include "Emu/SysCalls/Callback.h"

#include "Ini.h"
#include "Emu/FS/VFS.h"
#include "cellSysutil.h"

extern Module cellSysutil;

std::unique_ptr<sysutil_t> g_sysutil;

const char* get_systemparam_id_name(s32 id)
{
	switch (id)
	{
	case CELL_SYSUTIL_SYSTEMPARAM_ID_LANG: return "ID_LANG";
	case CELL_SYSUTIL_SYSTEMPARAM_ID_ENTER_BUTTON_ASSIGN: return "ID_ENTER_BUTTON_ASSIGN";
	case CELL_SYSUTIL_SYSTEMPARAM_ID_DATE_FORMAT: return "ID_DATE_FORMAT";
	case CELL_SYSUTIL_SYSTEMPARAM_ID_TIME_FORMAT: return "ID_TIME_FORMAT";
	case CELL_SYSUTIL_SYSTEMPARAM_ID_TIMEZONE: return "ID_TIMEZONE";
	case CELL_SYSUTIL_SYSTEMPARAM_ID_SUMMERTIME: return "ID_SUMMERTIME";
	case CELL_SYSUTIL_SYSTEMPARAM_ID_GAME_PARENTAL_LEVEL: return "ID_GAME_PARENTAL_LEVEL";
	case CELL_SYSUTIL_SYSTEMPARAM_ID_GAME_PARENTAL_LEVEL0_RESTRICT: return "ID_GAME_PARENTAL_LEVEL0_RESTRICT";
	case CELL_SYSUTIL_SYSTEMPARAM_ID_CURRENT_USER_HAS_NP_ACCOUNT: return "ID_CURRENT_USER_HAS_NP_ACCOUNT";
	case CELL_SYSUTIL_SYSTEMPARAM_ID_CAMERA_PLFREQ: return "ID_CAMERA_PLFREQ";
	case CELL_SYSUTIL_SYSTEMPARAM_ID_PAD_RUMBLE: return "ID_PAD_RUMBLE";
	case CELL_SYSUTIL_SYSTEMPARAM_ID_KEYBOARD_TYPE: return "ID_KEYBOARD_TYPE";
	case CELL_SYSUTIL_SYSTEMPARAM_ID_JAPANESE_KEYBOARD_ENTRY_METHOD: return "ID_JAPANESE_KEYBOARD_ENTRY_METHOD";
	case CELL_SYSUTIL_SYSTEMPARAM_ID_CHINESE_KEYBOARD_ENTRY_METHOD: return "ID_CHINESE_KEYBOARD_ENTRY_METHOD";
	case CELL_SYSUTIL_SYSTEMPARAM_ID_PAD_AUTOOFF: return "ID_PAD_AUTOOFF";
	case CELL_SYSUTIL_SYSTEMPARAM_ID_NICKNAME: return "ID_NICKNAME";
	case CELL_SYSUTIL_SYSTEMPARAM_ID_CURRENT_USERNAME: return "ID_CURRENT_USERNAME";
	default: return "???";
	}
}

s32 cellSysutilGetSystemParamInt(s32 id, vm::ptr<s32> value)
{
	cellSysutil.Warning("cellSysutilGetSystemParamInt(id=0x%x(%s), value=*0x%x)", id, get_systemparam_id_name(id), value);

	switch(id)
	{
	case CELL_SYSUTIL_SYSTEMPARAM_ID_LANG:
		*value = Ini.SysLanguage.GetValue();
	break;

	case CELL_SYSUTIL_SYSTEMPARAM_ID_ENTER_BUTTON_ASSIGN:
		*value = CELL_SYSUTIL_ENTER_BUTTON_ASSIGN_CROSS;
	break;

	case CELL_SYSUTIL_SYSTEMPARAM_ID_DATE_FORMAT:
		*value = CELL_SYSUTIL_DATE_FMT_DDMMYYYY;
	break;

	case CELL_SYSUTIL_SYSTEMPARAM_ID_TIME_FORMAT:
		*value = CELL_SYSUTIL_TIME_FMT_CLOCK24;
	break;

	case CELL_SYSUTIL_SYSTEMPARAM_ID_TIMEZONE:
		*value = 180;
	break;

	case CELL_SYSUTIL_SYSTEMPARAM_ID_SUMMERTIME:
		*value = 0;
	break;

	case CELL_SYSUTIL_SYSTEMPARAM_ID_GAME_PARENTAL_LEVEL:
		*value = CELL_SYSUTIL_GAME_PARENTAL_OFF;
	break;

	case CELL_SYSUTIL_SYSTEMPARAM_ID_GAME_PARENTAL_LEVEL0_RESTRICT:
		*value = CELL_SYSUTIL_GAME_PARENTAL_LEVEL0_RESTRICT_OFF;
	break;

	case CELL_SYSUTIL_SYSTEMPARAM_ID_CURRENT_USER_HAS_NP_ACCOUNT:
		*value = 0;
	break;

	case CELL_SYSUTIL_SYSTEMPARAM_ID_CAMERA_PLFREQ:
		*value = CELL_SYSUTIL_CAMERA_PLFREQ_DISABLED;
	break;

	case CELL_SYSUTIL_SYSTEMPARAM_ID_PAD_RUMBLE:
		*value = CELL_SYSUTIL_PAD_RUMBLE_OFF;
	break;

	case CELL_SYSUTIL_SYSTEMPARAM_ID_KEYBOARD_TYPE:
		*value = 0;
	break;

	case CELL_SYSUTIL_SYSTEMPARAM_ID_JAPANESE_KEYBOARD_ENTRY_METHOD:
		*value = 0;
	break;

	case CELL_SYSUTIL_SYSTEMPARAM_ID_CHINESE_KEYBOARD_ENTRY_METHOD:
		*value = 0;
	break;

	case CELL_SYSUTIL_SYSTEMPARAM_ID_PAD_AUTOOFF:
		*value = 0;
	break;

	default:
		return CELL_EINVAL;
	}

	return CELL_OK;
}

s32 cellSysutilGetSystemParamString(s32 id, vm::ptr<char> buf, u32 bufsize)
{
	cellSysutil.Log("cellSysutilGetSystemParamString(id=0x%x(%s), buf=*0x%x, bufsize=%d)", id, get_systemparam_id_name(id), buf, bufsize);

	memset(buf.get_ptr(), 0, bufsize);

	switch(id)
	{
	case CELL_SYSUTIL_SYSTEMPARAM_ID_NICKNAME:
		memcpy(buf.get_ptr(), "Unknown", 8); // for example
	break;

	case CELL_SYSUTIL_SYSTEMPARAM_ID_CURRENT_USERNAME:
		memcpy(buf.get_ptr(), "Unknown", 8);
	break;

	default:
		return CELL_EINVAL;
	}

	return CELL_OK;
}

struct sys_callback
{
	vm::ptr<CellSysutilCallback> func;
	vm::ptr<void> arg;
}
g_sys_callback[4];

void sysutilSendSystemCommand(u64 status, u64 param)
{
	// TODO: check it and find the source of the return value (void isn't equal to CELL_OK)
	for (auto& cb : g_sys_callback)
	{
		if (cb.func)
		{
			Emu.GetCallbackManager().Register([=](CPUThread& CPU) -> s32
			{
				cb.func(static_cast<PPUThread&>(CPU), status, param, cb.arg);
				return CELL_OK;
			});
		}
	}
}

s32 cellSysutilCheckCallback(PPUThread& CPU)
{
	cellSysutil.Log("cellSysutilCheckCallback()");

	while (auto func = Emu.GetCallbackManager().Check())
	{
		CHECK_EMU_STATUS;

		if (s32 res = func(CPU))
		{
			return res;
		}
	}

	return CELL_OK;
}

s32 cellSysutilRegisterCallback(s32 slot, vm::ptr<CellSysutilCallback> func, vm::ptr<void> userdata)
{
	cellSysutil.Warning("cellSysutilRegisterCallback(slot=%d, func=*0x%x, userdata=*0x%x)", slot, func, userdata);

	if ((u32)slot > 3)
	{
		return CELL_SYSUTIL_ERROR_VALUE;
	}

	g_sys_callback[slot].func = func;
	g_sys_callback[slot].arg = userdata;
	return CELL_OK;
}

s32 cellSysutilUnregisterCallback(s32 slot)
{
	cellSysutil.Warning("cellSysutilUnregisterCallback(slot=%d)", slot);

	if ((u32)slot > 3)
	{
		return CELL_SYSUTIL_ERROR_VALUE;
	}

	g_sys_callback[slot].func.set(0);
	g_sys_callback[slot].arg.set(0);
	return CELL_OK;
}

s32 cellSysCacheClear(void)
{
	cellSysutil.Todo("cellSysCacheClear()");

	if (g_sysutil->cacheMounted.exchange(false))
	{
		return CELL_SYSCACHE_ERROR_NOTMOUNTED;
	}

	std::string localPath;
	Emu.GetVFS().GetDevice(std::string("/dev_hdd1/cache/"), localPath);

	// TODO: Delete everything in the cache folder, except the README

	return CELL_SYSCACHE_RET_OK_CLEARED;
}

s32 cellSysCacheMount(vm::ptr<CellSysCacheParam> param)
{
	cellSysutil.Warning("cellSysCacheMount(param=*0x%x)", param);

	// TODO: implement
	char id[CELL_SYSCACHE_ID_SIZE];
	strncpy(id, param->cacheId, CELL_SYSCACHE_ID_SIZE);
	strncpy(param->getCachePath, ("/dev_hdd1/cache/" + std::string(id) + "/").c_str(), CELL_SYSCACHE_PATH_MAX);
	param->getCachePath[CELL_SYSCACHE_PATH_MAX - 1] = '\0';
	Emu.GetVFS().CreateDir(std::string(param->getCachePath));

	return CELL_SYSCACHE_RET_OK_RELAYED;
}

bool g_bgm_playback_enabled = true;

s32 cellSysutilEnableBgmPlayback()
{
	cellSysutil.Warning("cellSysutilEnableBgmPlayback()");

	// TODO
	g_bgm_playback_enabled = true;

	return CELL_OK;
}

s32 cellSysutilEnableBgmPlaybackEx(vm::ptr<CellSysutilBgmPlaybackExtraParam> param)
{
	cellSysutil.Warning("cellSysutilEnableBgmPlaybackEx(param=*0x%x)", param);

	// TODO
	g_bgm_playback_enabled = true; 

	return CELL_OK;
}

s32 cellSysutilDisableBgmPlayback()
{
	cellSysutil.Warning("cellSysutilDisableBgmPlayback()");

	// TODO
	g_bgm_playback_enabled = false;

	return CELL_OK;
}

s32 cellSysutilDisableBgmPlaybackEx(vm::ptr<CellSysutilBgmPlaybackExtraParam> param)
{
	cellSysutil.Warning("cellSysutilDisableBgmPlaybackEx(param=*0x%x)", param);

	// TODO
	g_bgm_playback_enabled = false;

	return CELL_OK;
}

s32 cellSysutilGetBgmPlaybackStatus(vm::ptr<CellSysutilBgmPlaybackStatus> status)
{
	cellSysutil.Warning("cellSysutilGetBgmPlaybackStatus(status=*0x%x)", status);

	// TODO
	status->playerState = CELL_SYSUTIL_BGMPLAYBACK_STATUS_STOP;
	status->enableState = g_bgm_playback_enabled ? CELL_SYSUTIL_BGMPLAYBACK_STATUS_ENABLE : CELL_SYSUTIL_BGMPLAYBACK_STATUS_DISABLE;
	status->currentFadeRatio = 0; // current volume ratio (0%)
	memset(status->contentId, 0, sizeof(status->contentId));
	memset(status->reserved, 0, sizeof(status->reserved));

	return CELL_OK;
}

s32 cellSysutilGetBgmPlaybackStatus2(vm::ptr<CellSysutilBgmPlaybackStatus2> status2)
{
	cellSysutil.Warning("cellSysutilGetBgmPlaybackStatus2(status2=*0x%x)", status2);

	// TODO
	status2->playerState = CELL_SYSUTIL_BGMPLAYBACK_STATUS_STOP;
	memset(status2->reserved, 0, sizeof(status2->reserved));

	return CELL_OK;
}

s32 cellSysutilSetBgmPlaybackExtraParam()
{
	throw EXCEPTION("");
}

s32 cellSysutilRegisterCallbackDispatcher()
{
	throw EXCEPTION("");
}

s32 cellSysutilPacketWrite()
{
	throw EXCEPTION("");
}

s32 cellSysutilGameDataAssignVmc()
{
	throw EXCEPTION("");
}

s32 cellSysutilGameDataExit()
{
	throw EXCEPTION("");
}

s32 cellSysutilGameExit_I()
{
	throw EXCEPTION("");
}

s32 cellSysutilGamePowerOff_I()
{
	throw EXCEPTION("");
}

s32 cellSysutilGameReboot_I()
{
	throw EXCEPTION("");
}

extern void cellSysutil_SaveData_init();
extern void cellSysutil_GameData_init();
extern void cellSysutil_MsgDialog_init();
extern void cellSysutil_OskDialog_init();
extern void cellSysutil_Storage_init();
extern void cellSysutil_Sysconf_init();
extern void cellSysutil_SysutilAvc_init();
extern void cellSysutil_WebBrowser_init();
extern void cellSysutil_AudioOut_init();
extern void cellSysutil_VideoOut_init();

Module cellSysutil("cellSysutil", []()
{
	g_sysutil = std::make_unique<sysutil_t>();

	for (auto& v : g_sys_callback)
	{
		v.func.set(0);
		v.arg.set(0);
	}

	cellSysutil_SaveData_init(); // cellSaveData functions
	cellSysutil_GameData_init(); // cellGameData, cellHddGame functions
	cellSysutil_MsgDialog_init(); // cellMsgDialog functions
	cellSysutil_OskDialog_init(); // cellOskDialog functions
	cellSysutil_Storage_init(); // cellStorage functions
	cellSysutil_Sysconf_init(); // cellSysconf functions
	cellSysutil_SysutilAvc_init(); // cellSysutilAvc functions
	cellSysutil_WebBrowser_init(); // cellWebBrowser, cellWebComponent functions
	cellSysutil_AudioOut_init(); // cellAudioOut functions
	cellSysutil_VideoOut_init(); // cellVideoOut functions

	REG_FUNC(cellSysutil, cellSysutilGetSystemParamInt);
	REG_FUNC(cellSysutil, cellSysutilGetSystemParamString);

	REG_FUNC(cellSysutil, cellSysutilCheckCallback);
	REG_FUNC(cellSysutil, cellSysutilRegisterCallback);
	REG_FUNC(cellSysutil, cellSysutilUnregisterCallback);

	REG_FUNC(cellSysutil, cellSysutilGetBgmPlaybackStatus);
	REG_FUNC(cellSysutil, cellSysutilGetBgmPlaybackStatus2);
	REG_FUNC(cellSysutil, cellSysutilEnableBgmPlayback);
	REG_FUNC(cellSysutil, cellSysutilEnableBgmPlaybackEx);
	REG_FUNC(cellSysutil, cellSysutilDisableBgmPlayback);
	REG_FUNC(cellSysutil, cellSysutilDisableBgmPlaybackEx);
	REG_FUNC(cellSysutil, cellSysutilSetBgmPlaybackExtraParam);

	REG_FUNC(cellSysutil, cellSysCacheMount);
	REG_FUNC(cellSysutil, cellSysCacheClear);

	REG_FUNC(cellSysutil, cellSysutilRegisterCallbackDispatcher);
	REG_FUNC(cellSysutil, cellSysutilPacketWrite);

	REG_FUNC(cellSysutil, cellSysutilGameDataAssignVmc);
	REG_FUNC(cellSysutil, cellSysutilGameDataExit);
	REG_FUNC(cellSysutil, cellSysutilGameExit_I);
	REG_FUNC(cellSysutil, cellSysutilGamePowerOff_I);
	REG_FUNC(cellSysutil, cellSysutilGameReboot_I);
});
