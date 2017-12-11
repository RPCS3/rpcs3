#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/IdManager.h"
#include "Emu/Cell/PPUModule.h"

#include "cellSysutil.h"

#include "Utilities/StrUtil.h"

#include <mutex>
#include <queue>

logs::channel cellSysutil("cellSysutil");

struct sysutil_cb_manager
{
	std::mutex mutex;

	std::array<std::pair<vm::ptr<CellSysutilCallback>, vm::ptr<void>>, 4> callbacks;

	std::queue<std::function<s32(ppu_thread&)>> registered;

	std::function<s32(ppu_thread&)> get_cb()
	{
		std::lock_guard<std::mutex> lock(mutex);

		if (registered.empty())
		{
			return nullptr;
		}

		auto func = std::move(registered.front());

		registered.pop();

		return func;
	}
};

extern void sysutil_register_cb(std::function<s32(ppu_thread&)>&& cb)
{
	const auto cbm = fxm::get_always<sysutil_cb_manager>();

	std::lock_guard<std::mutex> lock(cbm->mutex);

	cbm->registered.push(std::move(cb));
}

extern void sysutil_send_system_cmd(u64 status, u64 param)
{
	if (const auto cbm = fxm::get<sysutil_cb_manager>())
	{
		for (auto& cb : cbm->callbacks)
		{
			if (cb.first)
			{
				std::lock_guard<std::mutex> lock(cbm->mutex);

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
		case CELL_SYSUTIL_SYSTEMPARAM_ID_NICKNAME: return "ID_NICKNAME";
		case CELL_SYSUTIL_SYSTEMPARAM_ID_CURRENT_USERNAME: return "ID_CURRENT_USERNAME";
		}

		return unknown;
	});
}

s32 cellSysutilGetSystemParamInt(CellSysutilParamId id, vm::ptr<s32> value)
{
	cellSysutil.warning("cellSysutilGetSystemParamInt(id=0x%x(%s), value=*0x%x)", id, id, value);

	// TODO: load this information from config (preferably "sys/" group)

	switch (id)
	{
	case CELL_SYSUTIL_SYSTEMPARAM_ID_LANG:
		*value = g_cfg.sys.language;
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

s32 cellSysutilGetSystemParamString(CellSysutilParamId id, vm::ptr<char> buf, u32 bufsize)
{
	cellSysutil.trace("cellSysutilGetSystemParamString(id=0x%x(%s), buf=*0x%x, bufsize=%d)", id, id, buf, bufsize);

	memset(buf.get_ptr(), 0, bufsize);

	switch (id)
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

s32 cellSysutilCheckCallback(ppu_thread& ppu)
{
	cellSysutil.trace("cellSysutilCheckCallback()");

	const auto cbm = fxm::get_always<sysutil_cb_manager>();

	while (auto&& func = cbm->get_cb())
	{
		if (s32 res = func(ppu))
		{
			return res;
		}

		thread_ctrl::test();
	}

	return CELL_OK;
}

s32 cellSysutilRegisterCallback(s32 slot, vm::ptr<CellSysutilCallback> func, vm::ptr<void> userdata)
{
	cellSysutil.warning("cellSysutilRegisterCallback(slot=%d, func=*0x%x, userdata=*0x%x)", slot, func, userdata);

	if (slot >= 4)
	{
		return CELL_SYSUTIL_ERROR_VALUE;
	}

	const auto cbm = fxm::get_always<sysutil_cb_manager>();

	cbm->callbacks[slot] = std::make_pair(func, userdata);

	return CELL_OK;
}

s32 cellSysutilUnregisterCallback(u32 slot)
{
	cellSysutil.warning("cellSysutilUnregisterCallback(slot=%d)", slot);

	if (slot >= 4)
	{
		return CELL_SYSUTIL_ERROR_VALUE;
	}

	const auto cbm = fxm::get_always<sysutil_cb_manager>();

	cbm->callbacks[slot] = std::make_pair(vm::null, vm::null);

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
	verify(HERE), cache_id.size() < sizeof(param->cacheId);
	
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
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellSysutilUnregisterCallbackDispatcher()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellSysutilPacketRead()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellSysutilPacketWrite()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellSysutilPacketBegin()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellSysutilPacketEnd()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellSysutilGameDataAssignVmc()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellSysutilGameDataExit()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellSysutilGameExit_I()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellSysutilGamePowerOff_I()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellSysutilGameReboot_I()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellSysutilSharedMemoryAlloc()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellSysutilSharedMemoryFree()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 _ZN4cxml7Element11AppendChildERS0_()
{
	UNIMPLEMENTED_FUNC(cellSysutil);
	return CELL_OK;
}

s32 _ZN4cxml8DocumentC1Ev()
{
	UNIMPLEMENTED_FUNC(cellSysutil);
	return CELL_OK;
}

s32 _ZN4cxml8DocumentD1Ev()
{
	UNIMPLEMENTED_FUNC(cellSysutil);
	return CELL_OK;
}

s32 _ZN4cxml8Document5ClearEv()
{
	UNIMPLEMENTED_FUNC(cellSysutil);
	return CELL_OK;
}

s32 _ZN4cxml8Document5WriteEPFiPKvjPvES3_()
{
	UNIMPLEMENTED_FUNC(cellSysutil);
	return CELL_OK;
}

s32 _ZN4cxml8Document12RegisterFileEPKvjPNS_4FileE()
{
	UNIMPLEMENTED_FUNC(cellSysutil);
	return CELL_OK;
}

s32 _ZN4cxml8Document13CreateElementEPKciPNS_7ElementE()
{
	UNIMPLEMENTED_FUNC(cellSysutil);
	return CELL_OK;
}

s32 _ZN4cxml8Document14SetHeaderMagicEPKc()
{
	UNIMPLEMENTED_FUNC(cellSysutil);
	return CELL_OK;
}

s32 _ZN4cxml8Document16CreateFromBufferEPKvjb()
{
	UNIMPLEMENTED_FUNC(cellSysutil);
	return CELL_OK;
}

s32 _ZN4cxml8Document18GetDocumentElementEv()
{
	UNIMPLEMENTED_FUNC(cellSysutil);
	return CELL_OK;
}

s32 _ZNK4cxml4File7GetAddrEv()
{
	UNIMPLEMENTED_FUNC(cellSysutil);
	return CELL_OK;
}

s32 _ZNK4cxml7Element12GetAttributeEPKcPNS_9AttributeE()
{
	UNIMPLEMENTED_FUNC(cellSysutil);
	return CELL_OK;
}

s32 _ZNK4cxml7Element13GetFirstChildEv()
{
	UNIMPLEMENTED_FUNC(cellSysutil);
	return CELL_OK;
}

s32 _ZNK4cxml7Element14GetNextSiblingEv()
{
	UNIMPLEMENTED_FUNC(cellSysutil);
	return CELL_OK;
}

s32 _ZNK4cxml9Attribute6GetIntEPi()
{
	UNIMPLEMENTED_FUNC(cellSysutil);
	return CELL_OK;
}

s32 _ZNK4cxml9Attribute7GetFileEPNS_4FileE()
{
	UNIMPLEMENTED_FUNC(cellSysutil);
	return CELL_OK;
}

s32 _ZN8cxmlutil6SetIntERKN4cxml7ElementEPKci()
{
	UNIMPLEMENTED_FUNC(cellSysutil);
	return CELL_OK;
}

s32 _ZN8cxmlutil6GetIntERKN4cxml7ElementEPKcPi()
{
	UNIMPLEMENTED_FUNC(cellSysutil);
	return CELL_OK;
}

s32 _ZN8cxmlutil7SetFileERKN4cxml7ElementEPKcRKNS0_4FileE()
{
	UNIMPLEMENTED_FUNC(cellSysutil);
	return CELL_OK;
}

s32 _ZN8cxmlutil8GetFloatERKN4cxml7ElementEPKcPf()
{
	UNIMPLEMENTED_FUNC(cellSysutil);
	return CELL_OK;
}

s32 _ZN8cxmlutil9GetStringERKN4cxml7ElementEPKcPS5_Pj()
{
	UNIMPLEMENTED_FUNC(cellSysutil);
	return CELL_OK;
}

s32 _ZN8cxmlutil9SetStringERKN4cxml7ElementEPKcS5_()
{
	UNIMPLEMENTED_FUNC(cellSysutil);
	return CELL_OK;
}

s32 _ZN8cxmlutil16CheckElementNameERKN4cxml7ElementEPKc()
{
	UNIMPLEMENTED_FUNC(cellSysutil);
	return CELL_OK;
}

s32 _ZN8cxmlutil16FindChildElementERKN4cxml7ElementEPKcS5_S5_()
{
	UNIMPLEMENTED_FUNC(cellSysutil);
	return CELL_OK;
}

s32 _ZN8cxmlutil7GetFileERKN4cxml7ElementEPKcPNS0_4FileE()
{
	UNIMPLEMENTED_FUNC(cellSysutil);
	return CELL_OK;
}

s32 _ZN16sysutil_cxmlutil11FixedMemory3EndEi()
{
	UNIMPLEMENTED_FUNC(cellSysutil);
	return CELL_OK;
}

s32 _ZN16sysutil_cxmlutil11FixedMemory5BeginEi()
{
	UNIMPLEMENTED_FUNC(cellSysutil);
	return CELL_OK;
}

s32 _ZN16sysutil_cxmlutil11FixedMemory8AllocateEN4cxml14AllocationTypeEPvS3_jPS3_Pj()
{
	UNIMPLEMENTED_FUNC(cellSysutil);
	return CELL_OK;
}

s32 _ZN16sysutil_cxmlutil12PacketWriter5WriteEPKvjPv()
{
	UNIMPLEMENTED_FUNC(cellSysutil);
	return CELL_OK;
}

s32 _ZN16sysutil_cxmlutil12PacketWriterC1EiiRN4cxml8DocumentE()
{
	UNIMPLEMENTED_FUNC(cellSysutil);
	return CELL_OK;
}

s32 cellSysutil_E1EC7B6A(vm::ptr<u32> unk)
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
