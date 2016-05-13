#include "stdafx.h"
#include "Utilities/Config.h"
#include "Emu/System.h"
#include "Emu/IdManager.h"
#include "Emu/Cell/PPUModule.h"

#include "cellSysutil.h"

#include "Utilities/StrUtil.h"

logs::channel cellSysutil("cellSysutil", logs::level::notice);

// Temporarily
using sys_callbacks_t = std::array<std::pair<vm::ptr<CellSysutilCallback>, vm::ptr<void>>, 4>;

void sysutilSendSystemCommand(u64 status, u64 param)
{
	if (const auto g_sys_callback = fxm::get<sys_callbacks_t>())
	{
		for (auto& cb : *g_sys_callback)
		{
			if (cb.first)
			{
				Emu.GetCallbackManager().Register([=](PPUThread& ppu) -> s32
				{
					// TODO: check it and find the source of the return value (void isn't equal to CELL_OK)
					cb.first(ppu, status, param, cb.second);
					return CELL_OK;
				});
			}
		}
	}
}

cfg::map_entry<s32> g_cfg_sys_language(cfg::root.sys, "Language",
{
	{ "Japanese", CELL_SYSUTIL_LANG_JAPANESE },
	{ "English (US)", CELL_SYSUTIL_LANG_ENGLISH_US },
	{ "French", CELL_SYSUTIL_LANG_FRENCH },
	{ "Spanish", CELL_SYSUTIL_LANG_SPANISH },
	{ "German", CELL_SYSUTIL_LANG_GERMAN },
	{ "Italian", CELL_SYSUTIL_LANG_ITALIAN },
	{ "Dutch", CELL_SYSUTIL_LANG_DUTCH },
	{ "Portuguese (PT)", CELL_SYSUTIL_LANG_PORTUGUESE_PT },
	{ "Russian", CELL_SYSUTIL_LANG_RUSSIAN },
	{ "Korean", CELL_SYSUTIL_LANG_KOREAN },
	{ "Chinese (Trad.)", CELL_SYSUTIL_LANG_CHINESE_T },
	{ "Chinese (Simp.)", CELL_SYSUTIL_LANG_CHINESE_S },
	{ "Finnish", CELL_SYSUTIL_LANG_FINNISH },
	{ "Swedish", CELL_SYSUTIL_LANG_SWEDISH },
	{ "Danish", CELL_SYSUTIL_LANG_DANISH },
	{ "Norwegian", CELL_SYSUTIL_LANG_NORWEGIAN },
	{ "Polish", CELL_SYSUTIL_LANG_POLISH },
	{ "English (UK)", CELL_SYSUTIL_LANG_ENGLISH_GB },
});

enum class systemparam_id_name : s32 {};

template<>
struct unveil<systemparam_id_name, void>
{
	struct temp
	{
		s32 value;
		char buf[12];

		temp(systemparam_id_name value)
			: value(s32(value))
		{
		}
	};

	static inline const char* get(temp&& in)
	{
		switch (in.value)
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
		}

		std::snprintf(in.buf, sizeof(in.buf), "!0x%04X", in.value);
		return in.buf;
	}
};

s32 cellSysutilGetSystemParamInt(s32 id, vm::ptr<s32> value)
{
	cellSysutil.warning("cellSysutilGetSystemParamInt(id=%s, value=*0x%x)", systemparam_id_name(id), value);

	// TODO: load this information from config (preferably "sys/" group)

	switch(id)
	{
	case CELL_SYSUTIL_SYSTEMPARAM_ID_LANG:
		*value = g_cfg_sys_language.get();
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
	cellSysutil.trace("cellSysutilGetSystemParamString(id=0x%x(%s), buf=*0x%x, bufsize=%d)", id, systemparam_id_name(id), buf, bufsize);

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

s32 cellSysutilCheckCallback(PPUThread& CPU)
{
	cellSysutil.trace("cellSysutilCheckCallback()");

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
	cellSysutil.warning("cellSysutilRegisterCallback(slot=%d, func=*0x%x, userdata=*0x%x)", slot, func, userdata);

	if (slot >= sys_callbacks_t{}.size())
	{
		return CELL_SYSUTIL_ERROR_VALUE;
	}

	fxm::get_always<sys_callbacks_t>()->at(slot) = std::make_pair(func, userdata);
	return CELL_OK;
}

s32 cellSysutilUnregisterCallback(u32 slot)
{
	cellSysutil.warning("cellSysutilUnregisterCallback(slot=%d)", slot);

	if (slot >= sys_callbacks_t{}.size())
	{
		return CELL_SYSUTIL_ERROR_VALUE;
	}

	fxm::get_always<sys_callbacks_t>()->at(slot) = std::make_pair(vm::null, vm::null);
	return CELL_OK;
}

s32 cellSysCacheClear(void)
{
	cellSysutil.todo("cellSysCacheClear()");

	if (!fxm::check<CellSysCacheParam>())
	{
		return CELL_SYSCACHE_ERROR_NOTMOUNTED;
	}

	const std::string& local_path = vfs::get("/dev_hdd1/cache/");

	// TODO: Write tests to figure out, what is deleted.

	return CELL_SYSCACHE_RET_OK_CLEARED;
}

s32 cellSysCacheMount(vm::ptr<CellSysCacheParam> param)
{
	cellSysutil.warning("cellSysCacheMount(param=*0x%x)", param);

	const std::string& cache_id = param->cacheId;
	VERIFY(cache_id.size() < sizeof(param->cacheId));
	
	const std::string& cache_path = "/dev_hdd1/cache/" + cache_id + '/';
	strcpy_trunc(param->getCachePath, cache_path);

	// TODO: implement (what?)
	fs::create_dir(vfs::get(cache_path));
	fxm::make_always<CellSysCacheParam>(*param);

	return CELL_SYSCACHE_RET_OK_RELAYED;
}

bool g_bgm_playback_enabled = true;

s32 cellSysutilEnableBgmPlayback()
{
	cellSysutil.warning("cellSysutilEnableBgmPlayback()");

	// TODO
	g_bgm_playback_enabled = true;

	return CELL_OK;
}

s32 cellSysutilEnableBgmPlaybackEx(vm::ptr<CellSysutilBgmPlaybackExtraParam> param)
{
	cellSysutil.warning("cellSysutilEnableBgmPlaybackEx(param=*0x%x)", param);

	// TODO
	g_bgm_playback_enabled = true; 

	return CELL_OK;
}

s32 cellSysutilDisableBgmPlayback()
{
	cellSysutil.warning("cellSysutilDisableBgmPlayback()");

	// TODO
	g_bgm_playback_enabled = false;

	return CELL_OK;
}

s32 cellSysutilDisableBgmPlaybackEx(vm::ptr<CellSysutilBgmPlaybackExtraParam> param)
{
	cellSysutil.warning("cellSysutilDisableBgmPlaybackEx(param=*0x%x)", param);

	// TODO
	g_bgm_playback_enabled = false;

	return CELL_OK;
}

s32 cellSysutilGetBgmPlaybackStatus(vm::ptr<CellSysutilBgmPlaybackStatus> status)
{
	cellSysutil.warning("cellSysutilGetBgmPlaybackStatus(status=*0x%x)", status);

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
	cellSysutil.warning("cellSysutilGetBgmPlaybackStatus2(status2=*0x%x)", status2);

	// TODO
	status2->playerState = CELL_SYSUTIL_BGMPLAYBACK_STATUS_STOP;
	memset(status2->reserved, 0, sizeof(status2->reserved));

	return CELL_OK;
}

s32 cellSysutilSetBgmPlaybackExtraParam()
{
	cellSysutil.todo("cellSysutilSetBgmPlaybackExtraParam()");
	return CELL_OK;
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

DECLARE(ppu_module_manager::cellSysutil)("cellSysutil", []()
{
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
