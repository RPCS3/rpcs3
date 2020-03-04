#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/system_config.h"
#include "Emu/IdManager.h"
#include "Emu/VFS.h"
#include "Emu/Cell/PPUModule.h"

#include "Emu/Cell/lv2/sys_process.h"
#include "cellSysutil.h"

#include "Utilities/StrUtil.h"
#include "Utilities/lockless.h"
#include "Utilities/span.h"

LOG_CHANNEL(cellSysutil);

template<>
void fmt_class_string<CellSysutilError>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto error)
	{
		switch (error)
		{
			STR_CASE(CELL_SYSUTIL_ERROR_TYPE);
			STR_CASE(CELL_SYSUTIL_ERROR_VALUE);
			STR_CASE(CELL_SYSUTIL_ERROR_SIZE);
			STR_CASE(CELL_SYSUTIL_ERROR_NUM);
			STR_CASE(CELL_SYSUTIL_ERROR_BUSY);
			STR_CASE(CELL_SYSUTIL_ERROR_STATUS);
			STR_CASE(CELL_SYSUTIL_ERROR_MEMORY);
		}

		return unknown;
	});
}

struct sysutil_cb_manager
{
	struct alignas(8) registered_cb
	{
		vm::ptr<CellSysutilCallback> first;
		vm::ptr<void> second;
	};

	atomic_t<registered_cb> callbacks[4]{};

	lf_queue<std::function<s32(ppu_thread&)>> registered;
};

extern void sysutil_register_cb(std::function<s32(ppu_thread&)>&& cb)
{
	const auto cbm = g_fxo->get<sysutil_cb_manager>();

	cbm->registered.push(std::move(cb));
}

extern void sysutil_send_system_cmd(u64 status, u64 param)
{
	// May be nullptr if emulation is stopped
	if (const auto cbm = g_fxo->get<sysutil_cb_manager>())
	{
		for (sysutil_cb_manager::registered_cb cb : cbm->callbacks)
		{
			if (cb.first)
			{
				cbm->registered.push([=](ppu_thread& ppu) -> s32
				{
					// TODO: check it and find the source of the return value (void isn't equal to CELL_OK)
					cb.first(ppu, status, param, cb.second);
					return CELL_OK;
				});
			}
		}
	}
}

template <>
void fmt_class_string<CellSysutilLang>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](CellSysutilLang value)
	{
		switch (value)
		{
		case CELL_SYSUTIL_LANG_JAPANESE: return "Japanese";
		case CELL_SYSUTIL_LANG_ENGLISH_US: return "English (US)";
		case CELL_SYSUTIL_LANG_FRENCH: return "French";
		case CELL_SYSUTIL_LANG_SPANISH: return "Spanish";
		case CELL_SYSUTIL_LANG_GERMAN: return "German";
		case CELL_SYSUTIL_LANG_ITALIAN: return "Italian";
		case CELL_SYSUTIL_LANG_DUTCH: return "Dutch";
		case CELL_SYSUTIL_LANG_PORTUGUESE_PT: return "Portuguese (PT)";
		case CELL_SYSUTIL_LANG_RUSSIAN: return "Russian";
		case CELL_SYSUTIL_LANG_KOREAN: return "Korean";
		case CELL_SYSUTIL_LANG_CHINESE_T: return "Chinese (Trad.)";
		case CELL_SYSUTIL_LANG_CHINESE_S: return "Chinese (Simp.)";
		case CELL_SYSUTIL_LANG_FINNISH: return "Finnish";
		case CELL_SYSUTIL_LANG_SWEDISH: return "Swedish";
		case CELL_SYSUTIL_LANG_DANISH: return "Danish";
		case CELL_SYSUTIL_LANG_NORWEGIAN: return "Norwegian";
		case CELL_SYSUTIL_LANG_POLISH: return "Polish";
		case CELL_SYSUTIL_LANG_ENGLISH_GB: return "English (UK)";
		case CELL_SYSUTIL_LANG_PORTUGUESE_BR: return "Portuguese (BR)";
		case CELL_SYSUTIL_LANG_TURKISH: return "Turkish";
		}

		return unknown;
	});
}

template <>
void fmt_class_string<CellSysutilParamId>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto value)
	{
		switch (value)
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
		case CELL_SYSUTIL_SYSTEMPARAM_ID_MAGNETOMETER: return "ID_MAGNETOMETER";
		case CELL_SYSUTIL_SYSTEMPARAM_ID_NICKNAME: return "ID_NICKNAME";
		case CELL_SYSUTIL_SYSTEMPARAM_ID_CURRENT_USERNAME: return "ID_CURRENT_USERNAME";
		case CELL_SYSUTIL_SYSTEMPARAM_ID_x1008: return "ID_x1008";
		case CELL_SYSUTIL_SYSTEMPARAM_ID_x1011: return "ID_x1011";
		case CELL_SYSUTIL_SYSTEMPARAM_ID_x1012: return "ID_x1012";
		case CELL_SYSUTIL_SYSTEMPARAM_ID_x1024: return "ID_x1024";
		}

		return unknown;
	});
}

// Common string checks used in libsysutil functions
s32 sysutil_check_name_string(const char* src, s32 minlen, s32 maxlen)
{
	s32 lastpos;

	if (g_ps3_process_info.sdk_ver > 0x36FFFF)
	{
		// Limit null terminator boundary to before buffer max size
		lastpos = std::max(maxlen - 1, 0);
	}
	else
	{
		// Limit null terminator boundary to one after buffer max size
		lastpos = maxlen;
	}

	char cur = src[0];

	if (cur == '_')
	{
		// Invalid character at start
		return -1;
	}

	for (s32 index = 0;; cur = src[++index])
	{
		if (cur == '\0' || index == maxlen)
		{
			if (minlen > index || (maxlen == index && src[lastpos]))
			{
				// String length is invalid
				return -2;
			}

			// OK
			return 0;
		}

		if (cur >= 'A' && cur <= 'Z')
		{
			continue;
		}

		if (cur >= '0' && cur <= '9')
		{
			continue;
		}

		if (cur == '-' || cur == '_')
		{
			continue;
		}

		// Invalid character found
		return -1;
	}
}

error_code _cellSysutilGetSystemParamInt()
{
	UNIMPLEMENTED_FUNC(cellSysutil);
	return CELL_OK;
}

error_code cellSysutilGetSystemParamInt(CellSysutilParamId id, vm::ptr<s32> value)
{
	cellSysutil.warning("cellSysutilGetSystemParamInt(id=0x%x(%s), value=*0x%x)", id, id, value);

	if (!value)
	{
		return CELL_SYSUTIL_ERROR_VALUE;
	}

	// TODO: load this information from config (preferably "sys/" group)

	switch (id)
	{
	case CELL_SYSUTIL_SYSTEMPARAM_ID_LANG:
		*value = g_cfg.sys.language;
	break;

	case CELL_SYSUTIL_SYSTEMPARAM_ID_ENTER_BUTTON_ASSIGN:
		*value = static_cast<s32>(g_cfg.sys.enter_button_assignment.get());
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
		*value = CELL_SYSUTIL_PAD_RUMBLE_ON;
	break;

	case CELL_SYSUTIL_SYSTEMPARAM_ID_KEYBOARD_TYPE:
		*value = g_cfg.sys.keyboard_type;
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

	case CELL_SYSUTIL_SYSTEMPARAM_ID_MAGNETOMETER:
		*value = 0;
	break;

	default:
		return CELL_SYSUTIL_ERROR_VALUE;
	}

	return CELL_OK;
}

error_code cellSysutilGetSystemParamString(CellSysutilParamId id, vm::ptr<char> buf, u32 bufsize)
{
	cellSysutil.trace("cellSysutilGetSystemParamString(id=0x%x(%s), buf=*0x%x, bufsize=%d)", id, id, buf, bufsize);

	if (!buf)
	{
		return CELL_SYSUTIL_ERROR_VALUE;
	}

	u32 copy_size;
	std::string param_str = "Unknown";
	bool report_use = false;

	switch (id)
	{
	case CELL_SYSUTIL_SYSTEMPARAM_ID_NICKNAME:
	{
		copy_size = CELL_SYSUTIL_SYSTEMPARAM_NICKNAME_SIZE;
		break;
	}

	case CELL_SYSUTIL_SYSTEMPARAM_ID_CURRENT_USERNAME:
	{
		const fs::file username(vfs::get(fmt::format("/dev_hdd0/home/%08u/localusername", Emu.GetUsrId())));

		if (!username)
		{
			cellSysutil.error("cellSysutilGetSystemParamString(): Username for user %08u doesn't exist. Did you delete the username file?", Emu.GetUsrId());
		}
		else
		{
			// Read current username
			param_str = username.to_string();
		}

		copy_size = CELL_SYSUTIL_SYSTEMPARAM_CURRENT_USERNAME_SIZE;
		break;
	}

	case CELL_SYSUTIL_SYSTEMPARAM_ID_x1011: // Same as x1012
	case CELL_SYSUTIL_SYSTEMPARAM_ID_x1012: copy_size = 0x400; report_use = true; break;
	case CELL_SYSUTIL_SYSTEMPARAM_ID_x1024:	copy_size = 0x100; report_use = true; break;
	case CELL_SYSUTIL_SYSTEMPARAM_ID_x1008: copy_size = 0x4; report_use = true; break;
	default:
	{
		return CELL_SYSUTIL_ERROR_VALUE;
	}
	}

	if (bufsize != copy_size)
	{
		return CELL_SYSUTIL_ERROR_SIZE;
	}

	if (report_use)
	{
		cellSysutil.error("cellSysutilGetSystemParamString: Unknown ParamId 0x%x", id);
	}

	gsl::span dst(buf.get_ptr(), copy_size);
	strcpy_trunc(dst, param_str);
	return CELL_OK;
}

// Note: the way we do things here is inaccurate(but maybe sufficient)
// The real function goes over a table of 0x20 entries[ event_code:u32 callback_addr:u32 ]
// Those callbacks are registered through cellSysutilRegisterCallbackDispatcher(u32 event_code, vm::ptr<void> func_addr)
// The function goes through all the callback looking for one callback associated with event 0x100, if any is found it is called with parameters r3=0x101 r4=0
// This particular CB seems to be associated with sysutil itself
// Then it checks for events on an event_queue associated with sysutil, checks if any cb is associated with that event and calls them with parameters that come from the event
error_code cellSysutilCheckCallback(ppu_thread& ppu)
{
	cellSysutil.trace("cellSysutilCheckCallback()");

	const auto cbm = g_fxo->get<sysutil_cb_manager>();

	for (auto&& func : cbm->registered.pop_all())
	{
		if (s32 res = func(ppu))
		{
			// Currently impossible
			return not_an_error(res);
		}

		if (ppu.is_stopped())
		{
			return 0;
		}
	}

	return CELL_OK;
}

error_code cellSysutilRegisterCallback(s32 slot, vm::ptr<CellSysutilCallback> func, vm::ptr<void> userdata)
{
	cellSysutil.warning("cellSysutilRegisterCallback(slot=%d, func=*0x%x, userdata=*0x%x)", slot, func, userdata);

	if (slot >= 4)
	{
		return CELL_SYSUTIL_ERROR_VALUE;
	}

	const auto cbm = g_fxo->get<sysutil_cb_manager>();

	cbm->callbacks[slot].store({func, userdata});

	return CELL_OK;
}

error_code cellSysutilUnregisterCallback(u32 slot)
{
	cellSysutil.warning("cellSysutilUnregisterCallback(slot=%d)", slot);

	if (slot >= 4)
	{
		return CELL_SYSUTIL_ERROR_VALUE;
	}

	const auto cbm = g_fxo->get<sysutil_cb_manager>();

	cbm->callbacks[slot].store({});

	return CELL_OK;
}

bool g_bgm_playback_enabled = true;

error_code cellSysutilEnableBgmPlayback()
{
	cellSysutil.warning("cellSysutilEnableBgmPlayback()");

	// TODO
	g_bgm_playback_enabled = true;

	return CELL_OK;
}

error_code cellSysutilEnableBgmPlaybackEx(vm::ptr<CellSysutilBgmPlaybackExtraParam> param)
{
	cellSysutil.warning("cellSysutilEnableBgmPlaybackEx(param=*0x%x)", param);

	// TODO
	g_bgm_playback_enabled = true;

	return CELL_OK;
}

error_code cellSysutilDisableBgmPlayback()
{
	cellSysutil.warning("cellSysutilDisableBgmPlayback()");

	// TODO
	g_bgm_playback_enabled = false;

	return CELL_OK;
}

error_code cellSysutilDisableBgmPlaybackEx(vm::ptr<CellSysutilBgmPlaybackExtraParam> param)
{
	cellSysutil.warning("cellSysutilDisableBgmPlaybackEx(param=*0x%x)", param);

	// TODO
	g_bgm_playback_enabled = false;

	return CELL_OK;
}

error_code cellSysutilGetBgmPlaybackStatus(vm::ptr<CellSysutilBgmPlaybackStatus> status)
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

error_code cellSysutilGetBgmPlaybackStatus2(vm::ptr<CellSysutilBgmPlaybackStatus2> status2)
{
	cellSysutil.warning("cellSysutilGetBgmPlaybackStatus2(status2=*0x%x)", status2);

	// TODO
	status2->playerState = CELL_SYSUTIL_BGMPLAYBACK_STATUS_STOP;
	memset(status2->reserved, 0, sizeof(status2->reserved));

	return CELL_OK;
}

error_code cellSysutilSetBgmPlaybackExtraParam()
{
	cellSysutil.todo("cellSysutilSetBgmPlaybackExtraParam()");
	return CELL_OK;
}

error_code cellSysutilRegisterCallbackDispatcher()
{
	cellSysutil.todo("cellSysutilRegisterCallbackDispatcher()");
	return CELL_OK;
}

error_code cellSysutilUnregisterCallbackDispatcher()
{
	cellSysutil.todo("cellSysutilUnregisterCallbackDispatcher()");
	return CELL_OK;
}

error_code cellSysutilPacketRead()
{
	cellSysutil.todo("cellSysutilPacketRead()");
	return CELL_OK;
}

error_code cellSysutilPacketWrite()
{
	cellSysutil.todo("cellSysutilPacketWrite()");
	return CELL_OK;
}

error_code cellSysutilPacketBegin()
{
	cellSysutil.todo("cellSysutilPacketBegin()");
	return CELL_OK;
}

error_code cellSysutilPacketEnd()
{
	cellSysutil.todo("cellSysutilPacketEnd()");
	return CELL_OK;
}

error_code cellSysutilGameDataAssignVmc()
{
	cellSysutil.todo("cellSysutilGameDataAssignVmc()");
	return CELL_OK;
}

error_code cellSysutilGameDataExit()
{
	cellSysutil.todo("cellSysutilGameDataExit()");
	return CELL_OK;
}

error_code cellSysutilGameExit_I()
{
	cellSysutil.todo("cellSysutilGameExit_I()");
	return CELL_OK;
}

error_code cellSysutilGamePowerOff_I()
{
	cellSysutil.todo("cellSysutilGamePowerOff_I()");
	return CELL_OK;
}

error_code cellSysutilGameReboot_I()
{
	cellSysutil.todo("cellSysutilGameReboot_I()");
	return CELL_OK;
}

error_code cellSysutilSharedMemoryAlloc()
{
	cellSysutil.todo("cellSysutilSharedMemoryAlloc()");
	return CELL_OK;
}

error_code cellSysutilSharedMemoryFree()
{
	cellSysutil.todo("cellSysutilSharedMemoryFree()");
	return CELL_OK;
}

error_code cellSysutilNotification()
{
	cellSysutil.todo("cellSysutilNotification()");
	return CELL_OK;
}

error_code _ZN4cxml7Element11AppendChildERS0_()
{
	UNIMPLEMENTED_FUNC(cellSysutil);
	return CELL_OK;
}

error_code _ZN4cxml8DocumentC1Ev()
{
	UNIMPLEMENTED_FUNC(cellSysutil);
	return CELL_OK;
}

error_code _ZN4cxml8DocumentD1Ev()
{
	UNIMPLEMENTED_FUNC(cellSysutil);
	return CELL_OK;
}

error_code _ZN4cxml8Document5ClearEv()
{
	UNIMPLEMENTED_FUNC(cellSysutil);
	return CELL_OK;
}

error_code _ZN4cxml8Document5WriteEPFiPKvjPvES3_()
{
	UNIMPLEMENTED_FUNC(cellSysutil);
	return CELL_OK;
}

error_code _ZN4cxml8Document12RegisterFileEPKvjPNS_4FileE()
{
	UNIMPLEMENTED_FUNC(cellSysutil);
	return CELL_OK;
}

error_code _ZN4cxml8Document13CreateElementEPKciPNS_7ElementE()
{
	UNIMPLEMENTED_FUNC(cellSysutil);
	return CELL_OK;
}

error_code _ZN4cxml8Document14SetHeaderMagicEPKc()
{
	UNIMPLEMENTED_FUNC(cellSysutil);
	return CELL_OK;
}

error_code _ZN4cxml8Document16CreateFromBufferEPKvjb()
{
	UNIMPLEMENTED_FUNC(cellSysutil);
	return CELL_OK;
}

error_code _ZN4cxml8Document18GetDocumentElementEv()
{
	UNIMPLEMENTED_FUNC(cellSysutil);
	return CELL_OK;
}

error_code _ZNK4cxml4File7GetAddrEv()
{
	UNIMPLEMENTED_FUNC(cellSysutil);
	return CELL_OK;
}

error_code _ZNK4cxml7Element12GetAttributeEPKcPNS_9AttributeE()
{
	UNIMPLEMENTED_FUNC(cellSysutil);
	return CELL_OK;
}

error_code _ZNK4cxml7Element13GetFirstChildEv()
{
	UNIMPLEMENTED_FUNC(cellSysutil);
	return CELL_OK;
}

error_code _ZNK4cxml7Element14GetNextSiblingEv()
{
	UNIMPLEMENTED_FUNC(cellSysutil);
	return CELL_OK;
}

error_code _ZNK4cxml9Attribute6GetIntEPi()
{
	UNIMPLEMENTED_FUNC(cellSysutil);
	return CELL_OK;
}

error_code _ZNK4cxml9Attribute7GetFileEPNS_4FileE()
{
	UNIMPLEMENTED_FUNC(cellSysutil);
	return CELL_OK;
}

error_code _ZN8cxmlutil6SetIntERKN4cxml7ElementEPKci()
{
	UNIMPLEMENTED_FUNC(cellSysutil);
	return CELL_OK;
}

error_code _ZN8cxmlutil6GetIntERKN4cxml7ElementEPKcPi()
{
	UNIMPLEMENTED_FUNC(cellSysutil);
	return CELL_OK;
}

error_code _ZN8cxmlutil7SetFileERKN4cxml7ElementEPKcRKNS0_4FileE()
{
	UNIMPLEMENTED_FUNC(cellSysutil);
	return CELL_OK;
}

error_code _ZN8cxmlutil8GetFloatERKN4cxml7ElementEPKcPf()
{
	UNIMPLEMENTED_FUNC(cellSysutil);
	return CELL_OK;
}

error_code _ZN8cxmlutil8SetFloatERKN4cxml7ElementEPKcf()
{
	UNIMPLEMENTED_FUNC(cellSysutil);
	return CELL_OK;
}

error_code _ZN8cxmlutil9GetStringERKN4cxml7ElementEPKcPS5_Pj()
{
	UNIMPLEMENTED_FUNC(cellSysutil);
	return CELL_OK;
}

error_code _ZN8cxmlutil9SetStringERKN4cxml7ElementEPKcS5_()
{
	UNIMPLEMENTED_FUNC(cellSysutil);
	return CELL_OK;
}

error_code _ZN8cxmlutil16CheckElementNameERKN4cxml7ElementEPKc()
{
	UNIMPLEMENTED_FUNC(cellSysutil);
	return CELL_OK;
}

error_code _ZN8cxmlutil16FindChildElementERKN4cxml7ElementEPKcS5_S5_()
{
	UNIMPLEMENTED_FUNC(cellSysutil);
	return CELL_OK;
}

error_code _ZN8cxmlutil7GetFileERKN4cxml7ElementEPKcPNS0_4FileE()
{
	UNIMPLEMENTED_FUNC(cellSysutil);
	return CELL_OK;
}

error_code _ZN16sysutil_cxmlutil11FixedMemory3EndEi()
{
	UNIMPLEMENTED_FUNC(cellSysutil);
	return CELL_OK;
}

error_code _ZN16sysutil_cxmlutil11FixedMemory5BeginEi()
{
	UNIMPLEMENTED_FUNC(cellSysutil);
	return CELL_OK;
}

error_code _ZN16sysutil_cxmlutil11FixedMemory8AllocateEN4cxml14AllocationTypeEPvS3_jPS3_Pj()
{
	UNIMPLEMENTED_FUNC(cellSysutil);
	return CELL_OK;
}

error_code _ZN16sysutil_cxmlutil12PacketWriter5WriteEPKvjPv()
{
	UNIMPLEMENTED_FUNC(cellSysutil);
	return CELL_OK;
}

error_code _ZN16sysutil_cxmlutil12PacketWriterC1EiiRN4cxml8DocumentE()
{
	UNIMPLEMENTED_FUNC(cellSysutil);
	return CELL_OK;
}

error_code cellSysutil_E1EC7B6A(vm::ptr<u32> unk)
{
	cellSysutil.todo("cellSysutil_E1EC7B6A(unk=*0x%x)", unk);
	*unk = 0;
	return CELL_OK;
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
extern void cellSysutil_SysCache_init();

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
	cellSysutil_SysCache_init(); // cellSysCache functions

	REG_FUNC(cellSysutil, _cellSysutilGetSystemParamInt);
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

	REG_FUNC(cellSysutil, cellSysutilRegisterCallbackDispatcher);
	REG_FUNC(cellSysutil, cellSysutilUnregisterCallbackDispatcher);
	REG_FUNC(cellSysutil, cellSysutilPacketRead);
	REG_FUNC(cellSysutil, cellSysutilPacketWrite);
	REG_FUNC(cellSysutil, cellSysutilPacketBegin);
	REG_FUNC(cellSysutil, cellSysutilPacketEnd);

	REG_FUNC(cellSysutil, cellSysutilGameDataAssignVmc);
	REG_FUNC(cellSysutil, cellSysutilGameDataExit);
	REG_FUNC(cellSysutil, cellSysutilGameExit_I);
	REG_FUNC(cellSysutil, cellSysutilGamePowerOff_I);
	REG_FUNC(cellSysutil, cellSysutilGameReboot_I);

	REG_FUNC(cellSysutil, cellSysutilSharedMemoryAlloc);
	REG_FUNC(cellSysutil, cellSysutilSharedMemoryFree);

	REG_FUNC(cellSysutil, cellSysutilNotification);

	REG_FUNC(cellSysutil, _ZN4cxml7Element11AppendChildERS0_);

	REG_FUNC(cellSysutil, _ZN4cxml8DocumentC1Ev);
	REG_FUNC(cellSysutil, _ZN4cxml8DocumentD1Ev);
	REG_FUNC(cellSysutil, _ZN4cxml8Document5ClearEv);
	REG_FUNC(cellSysutil, _ZN4cxml8Document5WriteEPFiPKvjPvES3_);
	REG_FUNC(cellSysutil, _ZN4cxml8Document12RegisterFileEPKvjPNS_4FileE);
	REG_FUNC(cellSysutil, _ZN4cxml8Document13CreateElementEPKciPNS_7ElementE);
	REG_FUNC(cellSysutil, _ZN4cxml8Document14SetHeaderMagicEPKc);
	REG_FUNC(cellSysutil, _ZN4cxml8Document16CreateFromBufferEPKvjb);
	REG_FUNC(cellSysutil, _ZN4cxml8Document18GetDocumentElementEv);

	REG_FUNC(cellSysutil, _ZNK4cxml4File7GetAddrEv);
	REG_FUNC(cellSysutil, _ZNK4cxml7Element12GetAttributeEPKcPNS_9AttributeE);
	REG_FUNC(cellSysutil, _ZNK4cxml7Element13GetFirstChildEv);
	REG_FUNC(cellSysutil, _ZNK4cxml7Element14GetNextSiblingEv);
	REG_FUNC(cellSysutil, _ZNK4cxml9Attribute6GetIntEPi);
	REG_FUNC(cellSysutil, _ZNK4cxml9Attribute7GetFileEPNS_4FileE);

	REG_FUNC(cellSysutil, _ZN8cxmlutil6SetIntERKN4cxml7ElementEPKci);
	REG_FUNC(cellSysutil, _ZN8cxmlutil6GetIntERKN4cxml7ElementEPKcPi);
	REG_FUNC(cellSysutil, _ZN8cxmlutil7SetFileERKN4cxml7ElementEPKcRKNS0_4FileE);
	REG_FUNC(cellSysutil, _ZN8cxmlutil8GetFloatERKN4cxml7ElementEPKcPf);
	REG_FUNC(cellSysutil, _ZN8cxmlutil8SetFloatERKN4cxml7ElementEPKcf);
	REG_FUNC(cellSysutil, _ZN8cxmlutil9GetStringERKN4cxml7ElementEPKcPS5_Pj);
	REG_FUNC(cellSysutil, _ZN8cxmlutil9SetStringERKN4cxml7ElementEPKcS5_);
	REG_FUNC(cellSysutil, _ZN8cxmlutil16CheckElementNameERKN4cxml7ElementEPKc);
	REG_FUNC(cellSysutil, _ZN8cxmlutil16FindChildElementERKN4cxml7ElementEPKcS5_S5_);
	REG_FUNC(cellSysutil, _ZN8cxmlutil7GetFileERKN4cxml7ElementEPKcPNS0_4FileE);

	REG_FUNC(cellSysutil, _ZN16sysutil_cxmlutil11FixedMemory3EndEi);
	REG_FUNC(cellSysutil, _ZN16sysutil_cxmlutil11FixedMemory5BeginEi);
	REG_FUNC(cellSysutil, _ZN16sysutil_cxmlutil11FixedMemory8AllocateEN4cxml14AllocationTypeEPvS3_jPS3_Pj);
	REG_FUNC(cellSysutil, _ZN16sysutil_cxmlutil12PacketWriter5WriteEPKvjPv);
	REG_FUNC(cellSysutil, _ZN16sysutil_cxmlutil12PacketWriterC1EiiRN4cxml8DocumentE);

	REG_FNID(cellSysutil, 0xE1EC7B6A, cellSysutil_E1EC7B6A);
});
