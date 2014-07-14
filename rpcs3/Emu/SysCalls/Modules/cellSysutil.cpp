#include "stdafx.h"
#include "Utilities/Log.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/SysCalls/Modules.h"
#include "Emu/DbgCommand.h"
#include "Emu/FS/vfsFile.h"
#include "Emu/Audio/sysutil_audio.h"

#include "cellSysutil.h"
#include "cellSysutil_SaveData.h"
#include "cellMsgDialog.h"
#include "cellGame.h"

#include "Loader/PSF.h"

typedef void (*CellHddGameStatCallback)(mem_ptr_t<CellHddGameCBResult> cbResult, mem_ptr_t<CellHddGameStatGet> get, mem_ptr_t<CellHddGameStatSet> set);


//void cellSysutil_init();
//Module cellSysutil(0x0015, cellSysutil_init);
Module *cellSysutil = nullptr;

int cellSysutilGetSystemParamInt(int id, mem32_t value)
{
	cellSysutil->Log("cellSysutilGetSystemParamInt(id=0x%x, value_addr=0x%x)", id, value.GetAddr());

	if(!value.IsGood())
	{
		return CELL_EFAULT;
	}

	switch(id)
	{
	case CELL_SYSUTIL_SYSTEMPARAM_ID_LANG:
		cellSysutil->Warning("cellSysutilGetSystemParamInt: CELL_SYSUTIL_SYSTEMPARAM_ID_LANG");
		value = Ini.SysLanguage.GetValue();
	break;

	case CELL_SYSUTIL_SYSTEMPARAM_ID_ENTER_BUTTON_ASSIGN:
		cellSysutil->Warning("cellSysutilGetSystemParamInt: CELL_SYSUTIL_SYSTEMPARAM_ID_ENTER_BUTTON_ASSIGN");
		value = CELL_SYSUTIL_ENTER_BUTTON_ASSIGN_CROSS;
	break;

	case CELL_SYSUTIL_SYSTEMPARAM_ID_DATE_FORMAT:
		cellSysutil->Warning("cellSysutilGetSystemParamInt: CELL_SYSUTIL_SYSTEMPARAM_ID_DATE_FORMAT");
		value = CELL_SYSUTIL_DATE_FMT_DDMMYYYY;
	break;

	case CELL_SYSUTIL_SYSTEMPARAM_ID_TIME_FORMAT:
		cellSysutil->Warning("cellSysutilGetSystemParamInt: CELL_SYSUTIL_SYSTEMPARAM_ID_TIME_FORMAT");
		value = CELL_SYSUTIL_TIME_FMT_CLOCK24;
	break;

	case CELL_SYSUTIL_SYSTEMPARAM_ID_TIMEZONE:
		cellSysutil->Warning("cellSysutilGetSystemParamInt: CELL_SYSUTIL_SYSTEMPARAM_ID_TIMEZONE");
		value = 3;
	break;

	case CELL_SYSUTIL_SYSTEMPARAM_ID_SUMMERTIME:
		cellSysutil->Warning("cellSysutilGetSystemParamInt: CELL_SYSUTIL_SYSTEMPARAM_ID_SUMMERTIME");
		value = 1;
	break;

	case CELL_SYSUTIL_SYSTEMPARAM_ID_GAME_PARENTAL_LEVEL:
		cellSysutil->Warning("cellSysutilGetSystemParamInt: CELL_SYSUTIL_SYSTEMPARAM_ID_GAME_PARENTAL_LEVEL");
		value = CELL_SYSUTIL_GAME_PARENTAL_OFF;
	break;

	case CELL_SYSUTIL_SYSTEMPARAM_ID_GAME_PARENTAL_LEVEL0_RESTRICT:
		cellSysutil->Warning("cellSysutilGetSystemParamInt: CELL_SYSUTIL_SYSTEMPARAM_ID_GAME_PARENTAL_LEVEL0_RESTRICT");
		value = CELL_SYSUTIL_GAME_PARENTAL_LEVEL0_RESTRICT_OFF;
	break;

	case CELL_SYSUTIL_SYSTEMPARAM_ID_CURRENT_USER_HAS_NP_ACCOUNT:
		cellSysutil->Warning("cellSysutilGetSystemParamInt: CELL_SYSUTIL_SYSTEMPARAM_ID_CURRENT_USER_HAS_NP_ACCOUNT");
		value = 0;
	break;

	case CELL_SYSUTIL_SYSTEMPARAM_ID_CAMERA_PLFREQ:
		cellSysutil->Warning("cellSysutilGetSystemParamInt: CELL_SYSUTIL_SYSTEMPARAM_ID_CAMERA_PLFREQ");
		value = CELL_SYSUTIL_CAMERA_PLFREQ_DISABLED;
	break;

	case CELL_SYSUTIL_SYSTEMPARAM_ID_PAD_RUMBLE:
		cellSysutil->Warning("cellSysutilGetSystemParamInt: CELL_SYSUTIL_SYSTEMPARAM_ID_PAD_RUMBLE");
		value = CELL_SYSUTIL_PAD_RUMBLE_OFF;
	break;

	case CELL_SYSUTIL_SYSTEMPARAM_ID_KEYBOARD_TYPE:
		cellSysutil->Warning("cellSysutilGetSystemParamInt: CELL_SYSUTIL_SYSTEMPARAM_ID_KEYBOARD_TYPE");
		value = 0;
	break;

	case CELL_SYSUTIL_SYSTEMPARAM_ID_JAPANESE_KEYBOARD_ENTRY_METHOD:
		cellSysutil->Warning("cellSysutilGetSystemParamInt: CELL_SYSUTIL_SYSTEMPARAM_ID_JAPANESE_KEYBOARD_ENTRY_METHOD");
		value = 0;
	break;

	case CELL_SYSUTIL_SYSTEMPARAM_ID_CHINESE_KEYBOARD_ENTRY_METHOD:
		cellSysutil->Warning("cellSysutilGetSystemParamInt: CELL_SYSUTIL_SYSTEMPARAM_ID_CHINESE_KEYBOARD_ENTRY_METHOD");
		value = 0;
	break;

	case CELL_SYSUTIL_SYSTEMPARAM_ID_PAD_AUTOOFF:
		cellSysutil->Warning("cellSysutilGetSystemParamInt: CELL_SYSUTIL_SYSTEMPARAM_ID_PAD_AUTOOFF");
		value = 0;
	break;

	default:
		return CELL_EINVAL;
	}

	return CELL_OK;
}

int cellSysutilGetSystemParamString(s32 id, mem_list_ptr_t<u8> buf, u32 bufsize)
{
	cellSysutil->Log("cellSysutilGetSystemParamString(id=%d, buf_addr=0x%x, bufsize=%d)", id, buf.GetAddr(), bufsize);

	if (!buf.IsGood())
		return CELL_EFAULT;

	memset(buf, 0, bufsize);

	switch(id)
	{
	case CELL_SYSUTIL_SYSTEMPARAM_ID_NICKNAME:
		cellSysutil->Warning("cellSysutilGetSystemParamString: CELL_SYSUTIL_SYSTEMPARAM_ID_NICKNAME");
		memcpy(buf, "Unknown", 8); //for example
	break;

	case CELL_SYSUTIL_SYSTEMPARAM_ID_CURRENT_USERNAME:
		cellSysutil->Warning("cellSysutilGetSystemParamString: CELL_SYSUTIL_SYSTEMPARAM_ID_CURRENT_USERNAME");
		memcpy(buf, "Unknown", 8);
	break;

	default:
		return CELL_EINVAL;
	}

	return CELL_OK;
}

int cellVideoOutGetState(u32 videoOut, u32 deviceIndex, u32 state_addr)
{
	cellSysutil->Log("cellVideoOutGetState(videoOut=0x%x, deviceIndex=0x%x, state_addr=0x%x)", videoOut, deviceIndex, state_addr);

	if(deviceIndex) return CELL_VIDEO_OUT_ERROR_DEVICE_NOT_FOUND;

	CellVideoOutState state = {};

	switch(videoOut)
	{
		case CELL_VIDEO_OUT_PRIMARY:
		{
			state.colorSpace = Emu.GetGSManager().GetColorSpace();
			state.state = Emu.GetGSManager().GetState();
			state.displayMode.resolutionId = Emu.GetGSManager().GetInfo().mode.resolutionId;
			state.displayMode.scanMode = Emu.GetGSManager().GetInfo().mode.scanMode;
			state.displayMode.conversion = Emu.GetGSManager().GetInfo().mode.conversion;
			state.displayMode.aspect = Emu.GetGSManager().GetInfo().mode.aspect;
			state.displayMode.refreshRates = re(Emu.GetGSManager().GetInfo().mode.refreshRates);

			Memory.WriteData(state_addr, state);
		}
		return CELL_VIDEO_OUT_SUCCEEDED;

		case CELL_VIDEO_OUT_SECONDARY:
		{
			state.colorSpace = CELL_VIDEO_OUT_COLOR_SPACE_RGB;
			state.state = CELL_VIDEO_OUT_OUTPUT_STATE_ENABLED;

			Memory.WriteData(state_addr, state);
		}
		return CELL_VIDEO_OUT_SUCCEEDED;
	}

	return CELL_VIDEO_OUT_ERROR_UNSUPPORTED_VIDEO_OUT;
}

int cellVideoOutGetResolution(u32 resolutionId, mem_ptr_t<CellVideoOutResolution> resolution)
{
	cellSysutil->Log("cellVideoOutGetResolution(resolutionId=%d, resolution_addr=0x%x)",
		resolutionId, resolution.GetAddr());

	if (!resolution.IsGood())
		return CELL_VIDEO_OUT_ERROR_ILLEGAL_PARAMETER;
	
	u32 num = ResolutionIdToNum(resolutionId);
	if(!num)
		return CELL_EINVAL;

	resolution->width = ResolutionTable[num].width;
	resolution->height = ResolutionTable[num].height;

	return CELL_VIDEO_OUT_SUCCEEDED;
}

s32 cellVideoOutConfigure(u32 videoOut, u32 config_addr, u32 option_addr, u32 waitForEvent)
{
	cellSysutil->Warning("cellVideoOutConfigure(videoOut=%d, config_addr=0x%x, option_addr=0x%x, waitForEvent=0x%x)",
		videoOut, config_addr, option_addr, waitForEvent);

	if(!Memory.IsGoodAddr(config_addr, sizeof(CellVideoOutConfiguration)))
	{
		return CELL_EFAULT;
	}

	CellVideoOutConfiguration& config = (CellVideoOutConfiguration&)Memory[config_addr];

	switch(videoOut)
	{
	case CELL_VIDEO_OUT_PRIMARY:
		if(config.resolutionId)
		{
			Emu.GetGSManager().GetInfo().mode.resolutionId = config.resolutionId;
		}

		Emu.GetGSManager().GetInfo().mode.format = config.format;

		if(config.aspect)
		{
			Emu.GetGSManager().GetInfo().mode.aspect = config.aspect;
		}

		if(config.pitch)
		{
			Emu.GetGSManager().GetInfo().mode.pitch = re(config.pitch);
		}

		return CELL_VIDEO_OUT_SUCCEEDED;

	case CELL_VIDEO_OUT_SECONDARY:
		return CELL_VIDEO_OUT_SUCCEEDED;
	}

	return CELL_VIDEO_OUT_ERROR_UNSUPPORTED_VIDEO_OUT;
}

int cellVideoOutGetConfiguration(u32 videoOut, u32 config_addr, u32 option_addr)
{
	cellSysutil->Warning("cellVideoOutGetConfiguration(videoOut=%d, config_addr=0x%x, option_addr=0x%x)",
		videoOut, config_addr, option_addr);

	if(!Memory.IsGoodAddr(config_addr, sizeof(CellVideoOutConfiguration))) return CELL_EFAULT;

	CellVideoOutConfiguration config = {};

	switch(videoOut)
	{
	case CELL_VIDEO_OUT_PRIMARY:
		config.resolutionId = Emu.GetGSManager().GetInfo().mode.resolutionId;
		config.format = Emu.GetGSManager().GetInfo().mode.format;
		config.aspect = Emu.GetGSManager().GetInfo().mode.aspect;
		config.pitch = re(Emu.GetGSManager().GetInfo().mode.pitch);

		Memory.WriteData(config_addr, config);

		return CELL_VIDEO_OUT_SUCCEEDED;

	case CELL_VIDEO_OUT_SECONDARY:
		Memory.WriteData(config_addr, config);

		return CELL_VIDEO_OUT_SUCCEEDED;
	}

	return CELL_VIDEO_OUT_ERROR_UNSUPPORTED_VIDEO_OUT;
}

int cellVideoOutGetDeviceInfo(u32 videoOut, u32 deviceIndex, mem_ptr_t<CellVideoOutDeviceInfo> info)
{
	cellSysutil->Warning("cellVideoOutGetDeviceInfo(videoOut=%u, deviceIndex=%u, info_addr=0x%x)",
		videoOut, deviceIndex, info.GetAddr());

	if(deviceIndex) return CELL_VIDEO_OUT_ERROR_DEVICE_NOT_FOUND;

	// Use standard dummy values for now.
	info->portType = CELL_VIDEO_OUT_PORT_HDMI;
	info->colorSpace = Emu.GetGSManager().GetColorSpace();
	info->latency = 1000;
	info->availableModeCount = 1;
	info->state = CELL_VIDEO_OUT_DEVICE_STATE_AVAILABLE;
	info->rgbOutputRange = 1;
	info->colorInfo.blueX = 0xFFFF;
	info->colorInfo.blueY = 0xFFFF;
	info->colorInfo.greenX = 0xFFFF;
	info->colorInfo.greenY = 0xFFFF;
	info->colorInfo.redX = 0xFFFF;
	info->colorInfo.redY = 0xFFFF;
	info->colorInfo.whiteX = 0xFFFF;
	info->colorInfo.whiteY = 0xFFFF;
	info->colorInfo.gamma = 100;
	info->availableModes[0].aspect = 0;
	info->availableModes[0].conversion = 0;
	info->availableModes[0].refreshRates = 0xF;
	info->availableModes[0].resolutionId = 1;
	info->availableModes[0].scanMode = 0;
	
	return CELL_OK;
}

int cellVideoOutGetNumberOfDevice(u32 videoOut)
{
	cellSysutil->Warning("cellVideoOutGetNumberOfDevice(videoOut=%d)", videoOut);

	switch(videoOut)
	{
	case CELL_VIDEO_OUT_PRIMARY: return 1;
	case CELL_VIDEO_OUT_SECONDARY: return 0;
	}

	return CELL_VIDEO_OUT_ERROR_UNSUPPORTED_VIDEO_OUT;
}

int cellVideoOutGetResolutionAvailability(u32 videoOut, u32 resolutionId, u32 aspect, u32 option)
{
	cellSysutil->Warning("cellVideoOutGetResolutionAvailability(videoOut=%d, resolutionId=0x%x, option_addr=0x%x, aspect=0x%x, option=0x%x)",
		videoOut, resolutionId, aspect, option);

	if(!ResolutionIdToNum(resolutionId))
	{
		return CELL_EINVAL;
	}

	switch(videoOut)
	{
	case CELL_VIDEO_OUT_PRIMARY: return 1;
	case CELL_VIDEO_OUT_SECONDARY: return 0;
	}

	return CELL_VIDEO_OUT_ERROR_UNSUPPORTED_VIDEO_OUT;
}

int cellSysutilCheckCallback()
{
	cellSysutil->Log("cellSysutilCheckCallback()");

	Emu.GetCallbackManager().m_exit_callback.Check();

	CPUThread& thr = Emu.GetCallbackThread();

	while (thr.IsAlive())
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
		if (Emu.IsStopped())
		{
			LOG_WARNING(HLE, "cellSysutilCheckCallback() aborted");
			break;
		}
	}

	return CELL_OK;
}

int cellSysutilRegisterCallback(int slot, u64 func_addr, u64 userdata)
{
	cellSysutil->Warning("cellSysutilRegisterCallback(slot=%d, func_addr=0x%llx, userdata=0x%llx)", slot, func_addr, userdata);

	Emu.GetCallbackManager().m_exit_callback.Register(slot, func_addr, userdata);

	SendDbgCommand(DID_REGISTRED_CALLBACK);

	return CELL_OK;
}

int cellSysutilUnregisterCallback(int slot)
{
	cellSysutil->Warning("cellSysutilUnregisterCallback(slot=%d)", slot);

	Emu.GetCallbackManager().m_exit_callback.Unregister(slot);

	SendDbgCommand(DID_UNREGISTRED_CALLBACK);

	return CELL_OK;
}

int cellAudioOutGetSoundAvailability(u32 audioOut, u32 type, u32 fs, u32 option)
{
	cellSysutil->Warning("cellAudioOutGetSoundAvailability(audioOut=%d, type=%d, fs=0x%x, option=%d)",
		audioOut, type, fs, option);

	option = 0;

	int available = 2; // should be at least 2

	switch(fs)
	{
		case CELL_AUDIO_OUT_FS_32KHZ:
		case CELL_AUDIO_OUT_FS_44KHZ:
		case CELL_AUDIO_OUT_FS_48KHZ:
		case CELL_AUDIO_OUT_FS_88KHZ:
		case CELL_AUDIO_OUT_FS_96KHZ:
		case CELL_AUDIO_OUT_FS_176KHZ:
		case CELL_AUDIO_OUT_FS_192KHZ:
			break;

		default: return CELL_AUDIO_OUT_ERROR_UNSUPPORTED_SOUND_MODE;
	}

	switch(type)
	{
		case CELL_AUDIO_OUT_CODING_TYPE_LPCM: break;
		case CELL_AUDIO_OUT_CODING_TYPE_AC3: available = 0; break;
		case CELL_AUDIO_OUT_CODING_TYPE_DTS: available = 0; break;

		default: return CELL_AUDIO_OUT_ERROR_UNSUPPORTED_SOUND_MODE;
	}

	switch(audioOut)
	{
		case CELL_AUDIO_OUT_PRIMARY: return available;
		case CELL_AUDIO_OUT_SECONDARY: return 0;
	}

	return CELL_AUDIO_OUT_ERROR_ILLEGAL_CONFIGURATION;
}

int cellAudioOutGetSoundAvailability2(u32 audioOut, u32 type, u32 fs, u32 ch, u32 option)
{
	cellSysutil->Warning("cellAudioOutGetSoundAvailability2(audioOut=%d, type=%d, fs=0x%x, ch=%d, option=%d)",
		audioOut, type, fs, ch, option);

	option = 0;

	int available = 2; // should be at least 2

	switch(fs)
	{
		case CELL_AUDIO_OUT_FS_32KHZ:
		case CELL_AUDIO_OUT_FS_44KHZ:
		case CELL_AUDIO_OUT_FS_48KHZ:
		case CELL_AUDIO_OUT_FS_88KHZ:
		case CELL_AUDIO_OUT_FS_96KHZ:
		case CELL_AUDIO_OUT_FS_176KHZ:
		case CELL_AUDIO_OUT_FS_192KHZ:
			break;

		default: return CELL_AUDIO_OUT_ERROR_UNSUPPORTED_SOUND_MODE;
	}

	switch(ch)
	{
		case 2: break;
		case 6: available = 0; break;
		case 8: available = 0; break;

		default: return CELL_AUDIO_OUT_ERROR_UNSUPPORTED_SOUND_MODE;
	}

	switch(type)
	{
		case CELL_AUDIO_OUT_CODING_TYPE_LPCM: break;
		case CELL_AUDIO_OUT_CODING_TYPE_AC3: available = 0; break;
		case CELL_AUDIO_OUT_CODING_TYPE_DTS: available = 0; break;

		default: return CELL_AUDIO_OUT_ERROR_UNSUPPORTED_SOUND_MODE;
	}

	switch(audioOut)
	{
		case CELL_AUDIO_OUT_PRIMARY: return available;
		case CELL_AUDIO_OUT_SECONDARY: return 0;
	}

	return CELL_AUDIO_OUT_ERROR_ILLEGAL_CONFIGURATION;
}

int cellAudioOutGetState(u32 audioOut, u32 deviceIndex, u32 state_addr)
{
	cellSysutil->Warning("cellAudioOutGetState(audioOut=0x%x,deviceIndex=0x%x,state_addr=0x%x)",audioOut,deviceIndex,state_addr);
	CellAudioOutState state = {};

	switch(audioOut)
	{
		case CELL_AUDIO_OUT_PRIMARY:
		{
			state.state = Emu.GetAudioManager().GetState();
			state.soundMode.type = Emu.GetAudioManager().GetInfo().mode.type;
			state.soundMode.channel = Emu.GetAudioManager().GetInfo().mode.channel;
			state.soundMode.fs = Emu.GetAudioManager().GetInfo().mode.fs;
			state.soundMode.layout = Emu.GetAudioManager().GetInfo().mode.layout;

			Memory.WriteData(state_addr, state);
		}
		return CELL_AUDIO_OUT_SUCCEEDED;

		case CELL_AUDIO_OUT_SECONDARY:
		{
			state.state = CELL_AUDIO_OUT_OUTPUT_STATE_ENABLED;

			Memory.WriteData(state_addr, state);
		}
		return CELL_AUDIO_OUT_SUCCEEDED;
	}

	return CELL_AUDIO_OUT_ERROR_UNSUPPORTED_AUDIO_OUT;
}

int cellAudioOutConfigure(u32 audioOut, mem_ptr_t<CellAudioOutConfiguration> config, mem_ptr_t<CellAudioOutOption> option, u32 waitForEvent)
{
	cellSysutil->Warning("cellAudioOutConfigure(audioOut=%d, config_addr=0x%x, option_addr=0x%x, (!)waitForEvent=%d)",
		audioOut, config.GetAddr(), option.GetAddr(), waitForEvent);

	if (!config.IsGood())
	{
		return CELL_EFAULT;
	}

	switch(audioOut)
	{
	case CELL_AUDIO_OUT_PRIMARY:
		if (config->channel)
		{
			Emu.GetAudioManager().GetInfo().mode.channel = config->channel;
		}

		Emu.GetAudioManager().GetInfo().mode.encoder = config->encoder;

		if(config->downMixer)
		{
			Emu.GetAudioManager().GetInfo().mode.downMixer = config->downMixer;
		}

		return CELL_AUDIO_OUT_SUCCEEDED;

	case CELL_AUDIO_OUT_SECONDARY:
		return CELL_AUDIO_OUT_SUCCEEDED;
	}

	return CELL_AUDIO_OUT_ERROR_UNSUPPORTED_AUDIO_OUT;
}

int cellAudioOutGetConfiguration(u32 audioOut, u32 config_addr, u32 option_addr)
{
	cellSysutil->Warning("cellAudioOutGetConfiguration(audioOut=%d, config_addr=0x%x, option_addr=0x%x)",
		audioOut, config_addr, option_addr);

	if(!Memory.IsGoodAddr(config_addr, sizeof(CellAudioOutConfiguration))) return CELL_EFAULT;

	CellAudioOutConfiguration config = {};

	switch(audioOut)
	{
		case CELL_AUDIO_OUT_PRIMARY:
		config.channel = Emu.GetAudioManager().GetInfo().mode.channel;
		config.encoder = Emu.GetAudioManager().GetInfo().mode.encoder;
		config.downMixer = Emu.GetAudioManager().GetInfo().mode.downMixer;

		Memory.WriteData(config_addr, config);

		return CELL_AUDIO_OUT_SUCCEEDED;

	case CELL_AUDIO_OUT_SECONDARY:
		Memory.WriteData(config_addr, config);

		return CELL_AUDIO_OUT_SUCCEEDED;
	}

	return CELL_AUDIO_OUT_ERROR_UNSUPPORTED_AUDIO_OUT;
}

int cellAudioOutGetNumberOfDevice(u32 audioOut)
{
	cellSysutil->Warning("cellAudioOutGetNumberOfDevice(videoOut=%d)",audioOut);

	switch(audioOut)
	{
		case CELL_AUDIO_OUT_PRIMARY: return 1;
		case CELL_AUDIO_OUT_SECONDARY: return 0;
	}

	return CELL_AUDIO_OUT_ERROR_UNSUPPORTED_AUDIO_OUT;
}

int cellAudioOutGetDeviceInfo(u32 audioOut, u32 deviceIndex, mem_ptr_t<CellAudioOutDeviceInfo> info)
{
	cellSysutil->Error("Unimplemented function: cellAudioOutGetDeviceInfo(audioOut=%u, deviceIndex=%u, info_addr=0x%x)",
		audioOut, deviceIndex, info.GetAddr());

	if(deviceIndex) return CELL_AUDIO_OUT_ERROR_DEVICE_NOT_FOUND;

	info->portType = CELL_AUDIO_OUT_PORT_HDMI;
	info->availableModeCount = 1;
	info->state = CELL_AUDIO_OUT_DEVICE_STATE_AVAILABLE;
	info->latency = 1000;
	info->availableModes[0].type = CELL_AUDIO_IN_CODING_TYPE_LPCM;
	info->availableModes[0].channel = CELL_AUDIO_OUT_CHNUM_2;
	info->availableModes[0].fs = CELL_AUDIO_OUT_FS_48KHZ;
	info->availableModes[0].layout = CELL_AUDIO_OUT_SPEAKER_LAYOUT_2CH;

	return CELL_AUDIO_OUT_SUCCEEDED;
}

int cellAudioOutSetCopyControl(u32 audioOut, u32 control)
{
	cellSysutil->Warning("cellAudioOutSetCopyControl(audioOut=%d,control=%d)",audioOut,control);

	switch(audioOut)
	{
		case CELL_AUDIO_OUT_PRIMARY:
		case CELL_AUDIO_OUT_SECONDARY:
			break;

		default: return CELL_AUDIO_OUT_ERROR_UNSUPPORTED_AUDIO_OUT;
	}

	switch(control)
	{
		case CELL_AUDIO_OUT_COPY_CONTROL_COPY_FREE:
		case CELL_AUDIO_OUT_COPY_CONTROL_COPY_ONCE:
		case CELL_AUDIO_OUT_COPY_CONTROL_COPY_NEVER:
			break;
			
		default: return CELL_AUDIO_OUT_ERROR_ILLEGAL_PARAMETER;
	}

	return CELL_AUDIO_OUT_SUCCEEDED;
}




typedef struct{
	char cacheId[CELL_SYSCACHE_ID_SIZE];
	char getCachePath[CELL_SYSCACHE_PATH_MAX];
	mem_ptr_t<void> reserved;
} CellSysCacheParam;


//class WxDirDeleteTraverser : public wxDirTraverser
//{
//public:
//	virtual wxDirTraverseResult OnFile(const wxString& filename)
//	{
//		if (!wxRemoveFile(filename)){
//			cellSysutil->Error("Couldn't delete File: %s", fmt::ToUTF8(filename).c_str());
//		}
//		return wxDIR_CONTINUE;
//	}
//	virtual wxDirTraverseResult OnDir(const wxString& dirname)
//	{
//		wxDir dir(dirname);
//		dir.Traverse(*this);
//		if (!wxRmDir(dirname)){
//		//this get triggered a few times while clearing folders
//		//but if this gets reimplented we should probably warn
//		//if directories can't be removed
//		}
//		return wxDIR_CONTINUE;
//	}
//};

int cellSysCacheClear(void)
{
	//if some software expects CELL_SYSCACHE_ERROR_NOTMOUNTED we need to check whether
	//it was mounted before, for that we would need to save the state which I don't know
	//where to put
	std::string localPath;
	Emu.GetVFS().GetDevice(std::string("/dev_hdd1/cache/"), localPath);
	
	//TODO: implement
	//if (rDirExists(localPath)){
	//	WxDirDeleteTraverser deleter;
	//	wxString f = wxFindFirstFile(fmt::FromUTF8(localPath+"\\*"),wxDIR);
	//	while (!f.empty())
	//	{
	//		wxDir dir(f);
	//		dir.Traverse(deleter);
	//		f = wxFindNextFile();
	//	}
	//	return CELL_SYSCACHE_RET_OK_CLEARED;
	//}
	//else{
	//	return CELL_SYSCACHE_ERROR_ACCESS_ERROR;
	//}

	return CELL_SYSCACHE_RET_OK_CLEARED;
}

int cellSysCacheMount(mem_ptr_t<CellSysCacheParam> param)
{
	//TODO: implement
	char id[CELL_SYSCACHE_ID_SIZE];
	strncpy(id, param->cacheId, CELL_SYSCACHE_ID_SIZE);
	strncpy(param->getCachePath, ("/dev_hdd1/cache/" + std::string(id) + "/").c_str(), CELL_SYSCACHE_PATH_MAX);
	param->getCachePath[CELL_SYSCACHE_PATH_MAX - 1] = '\0';
	Emu.GetVFS().CreateDir(std::string(param->getCachePath));

	return CELL_SYSCACHE_RET_OK_RELAYED;
}

int cellHddGameCheck(u32 version, u32 dirName_addr, u32 errDialog, mem_func_ptr_t<CellHddGameStatCallback> funcStat, u32 container)
{
	cellSysutil->Warning("cellHddGameCheck(version=%d, dirName_addr=0x%xx, errDialog=%d, funcStat_addr=0x%x, container=%d)",
		version, dirName_addr, errDialog, funcStat, container);

	if (!Memory.IsGoodAddr(dirName_addr) || !funcStat.IsGood())
		return CELL_HDDGAME_ERROR_PARAM;

	std::string dirName = Memory.ReadString(dirName_addr);
	if (dirName.size() != 9)
		return CELL_HDDGAME_ERROR_PARAM;

	MemoryAllocator<CellHddGameSystemFileParam> param;
	MemoryAllocator<CellHddGameCBResult> result;
	MemoryAllocator<CellHddGameStatGet> get;
	MemoryAllocator<CellHddGameStatSet> set;

	get->hddFreeSizeKB = 40 * 1024 * 1024; // 40 GB, TODO: Use the free space of the computer's HDD where RPCS3 is being run.
	get->isNewData = CELL_HDDGAME_ISNEWDATA_EXIST;
	get->sysSizeKB = 0; // TODO
	get->st_atime__  = 0; // TODO
	get->st_ctime__  = 0; // TODO
	get->st_mtime__  = 0; // TODO
	get->sizeKB = CELL_HDDGAME_SIZEKB_NOTCALC;
	memcpy(get->contentInfoPath, ("/dev_hdd0/game/"+dirName).c_str(), CELL_HDDGAME_PATH_MAX);
	memcpy(get->hddGamePath, ("/dev_hdd0/game/"+dirName+"/USRDIR").c_str(), CELL_HDDGAME_PATH_MAX);

	if (!Emu.GetVFS().ExistsDir(("/dev_hdd0/game/"+dirName).c_str()))
	{
		get->isNewData = CELL_HDDGAME_ISNEWDATA_NODIR;
	}
	else
	{
		// TODO: Is cellHddGameCheck really responsible for writing the information in get->getParam ? (If not, delete this else)

		vfsFile f(("/dev_hdd0/game/"+dirName+"/PARAM.SFO").c_str());
		PSFLoader psf(f);
		if (!psf.Load(false)) {
			return CELL_HDDGAME_ERROR_BROKEN;
		}

		get->getParam.parentalLevel = psf.GetInteger("PARENTAL_LEVEL");
		get->getParam.attribute = psf.GetInteger("ATTRIBUTE");
		get->getParam.resolution = psf.GetInteger("RESOLUTION");
		get->getParam.soundFormat = psf.GetInteger("SOUND_FORMAT");
		std::string title = psf.GetString("TITLE");
		memcpy(get->getParam.title, title.c_str(), std::min<size_t>(CELL_HDDGAME_SYSP_TITLE_SIZE,title.length()+1));
		std::string app_ver = psf.GetString("APP_VER");
		memcpy(get->getParam.dataVersion, app_ver.c_str(), std::min<size_t>(CELL_HDDGAME_SYSP_VERSION_SIZE,app_ver.length()+1));
		memcpy(get->getParam.titleId, dirName.c_str(), std::min<size_t>(CELL_HDDGAME_SYSP_TITLEID_SIZE,dirName.length()+1));

		for (u32 i=0; i<CELL_HDDGAME_SYSP_LANGUAGE_NUM; i++) {
			char key [16];
			sprintf(key, "TITLE_%02d", i);
			title = psf.GetString(key);
			memcpy(get->getParam.titleLang[i], title.c_str(), std::min<size_t>(CELL_HDDGAME_SYSP_TITLE_SIZE, title.length() + 1));
		}
	}

	// TODO ?

	funcStat(result.GetAddr(), get.GetAddr(), set.GetAddr());
	if (result->result != CELL_HDDGAME_CBRESULT_OK &&
		result->result != CELL_HDDGAME_CBRESULT_OK_CANCEL)
		return CELL_HDDGAME_ERROR_CBRESULT;

	// TODO ?

	return CELL_OK;
}

bool bgm_playback_enabled = true;

int cellSysutilEnableBgmPlayback()
{
	cellSysutil->Warning("cellSysutilEnableBgmPlayback()");

	// TODO
	bgm_playback_enabled = true;

	return CELL_OK;
}

int cellSysutilEnableBgmPlaybackEx(mem_ptr_t<CellSysutilBgmPlaybackExtraParam> param)
{
	cellSysutil->Warning("cellSysutilEnableBgmPlaybackEx(param_addr=0x%x)", param.GetAddr());

	// TODO
	bgm_playback_enabled = true; 

	return CELL_OK;
}

int cellSysutilDisableBgmPlayback()
{
	cellSysutil->Warning("cellSysutilDisableBgmPlayback()");

	// TODO
	bgm_playback_enabled = false;

	return CELL_OK;
}

int cellSysutilDisableBgmPlaybackEx(mem_ptr_t<CellSysutilBgmPlaybackExtraParam> param)
{
	cellSysutil->Warning("cellSysutilDisableBgmPlaybackEx(param_addr=0x%x)", param.GetAddr());

	// TODO
	bgm_playback_enabled = false;

	return CELL_OK;
}

int cellSysutilGetBgmPlaybackStatus(mem_ptr_t<CellSysutilBgmPlaybackStatus> status)
{
	cellSysutil->Log("cellSysutilGetBgmPlaybackStatus(status_addr=0x%x)", status.GetAddr());

	// TODO
	status->playerState = CELL_SYSUTIL_BGMPLAYBACK_STATUS_STOP;
	status->enableState = bgm_playback_enabled ? CELL_SYSUTIL_BGMPLAYBACK_STATUS_ENABLE : CELL_SYSUTIL_BGMPLAYBACK_STATUS_DISABLE;
	status->currentFadeRatio = 0; // current volume ratio (0%)
	memset(status->contentId, 0, sizeof(status->contentId));
	memset(status->reserved, 0, sizeof(status->reserved));

	return CELL_OK;
}

int cellSysutilGetBgmPlaybackStatus2(mem_ptr_t<CellSysutilBgmPlaybackStatus2> status2)
{
	cellSysutil->Log("cellSysutilGetBgmPlaybackStatus2(status2_addr=0x%x)", status2.GetAddr());

	// TODO
	status2->playerState = CELL_SYSUTIL_BGMPLAYBACK_STATUS_STOP;
	memset(status2->reserved, 0, sizeof(status2->reserved));

	return CELL_OK;
}

int cellWebBrowserEstimate2(mem8_ptr_t _config, mem32_ptr_t memSize)
{
	// TODO: When cellWebBrowser stuff is implemented, change this to some real
	// needed memory buffer size.
	*memSize = 1024 * 1024 * 1; // 1 MB
	return CELL_OK;
}

extern int cellGameDataCheckCreate2(u32 version, const mem_list_ptr_t<u8> dirName, u32 errDialog,
	mem_func_ptr_t<void(*)(mem_ptr_t<CellGameDataCBResult> cbResult, mem_ptr_t<CellGameDataStatGet> get, mem_ptr_t<CellGameDataStatSet> set)> funcStat, u32 container);

extern int cellGameDataCheckCreate(u32 version, const mem_list_ptr_t<u8> dirName, u32 errDialog,
	mem_func_ptr_t<void(*)(mem_ptr_t<CellGameDataCBResult> cbResult, mem_ptr_t<CellGameDataStatGet> get, mem_ptr_t<CellGameDataStatSet> set)> funcStat, u32 container);

void cellSysutil_init()
{
	cellSysutil->AddFunc(0x40e895d3, cellSysutilGetSystemParamInt);
	cellSysutil->AddFunc(0x938013a0, cellSysutilGetSystemParamString);

	cellSysutil->AddFunc(0x887572d5, cellVideoOutGetState);
	cellSysutil->AddFunc(0xe558748d, cellVideoOutGetResolution);
	cellSysutil->AddFunc(0x0bae8772, cellVideoOutConfigure);
	cellSysutil->AddFunc(0x15b0b0cd, cellVideoOutGetConfiguration);
	cellSysutil->AddFunc(0x1e930eef, cellVideoOutGetDeviceInfo);
	cellSysutil->AddFunc(0x75bbb672, cellVideoOutGetNumberOfDevice);
	cellSysutil->AddFunc(0xa322db75, cellVideoOutGetResolutionAvailability);

	cellSysutil->AddFunc(0x189a74da, cellSysutilCheckCallback);
	cellSysutil->AddFunc(0x9d98afa0, cellSysutilRegisterCallback);
	cellSysutil->AddFunc(0x02ff3c1b, cellSysutilUnregisterCallback);

	cellSysutil->AddFunc(0x7603d3db, cellMsgDialogOpen2);
	cellSysutil->AddFunc(0x3e22cb4b, cellMsgDialogOpenErrorCode);
	cellSysutil->AddFunc(0x9d6af72a, cellMsgDialogProgressBarSetMsg);
	cellSysutil->AddFunc(0x7bc2c8a8, cellMsgDialogProgressBarReset);
	cellSysutil->AddFunc(0x94862702, cellMsgDialogProgressBarInc);
	cellSysutil->AddFunc(0x20543730, cellMsgDialogClose);
	cellSysutil->AddFunc(0x62b0f803, cellMsgDialogAbort);

	cellSysutil->AddFunc(0xf4e3caa0, cellAudioOutGetState);
	cellSysutil->AddFunc(0x4692ab35, cellAudioOutConfigure);
	cellSysutil->AddFunc(0xc01b4e7c, cellAudioOutGetSoundAvailability);
	cellSysutil->AddFunc(0x2beac488, cellAudioOutGetSoundAvailability2);
	cellSysutil->AddFunc(0x7663e368, cellAudioOutGetDeviceInfo);
	cellSysutil->AddFunc(0xe5e2b09d, cellAudioOutGetNumberOfDevice);
	cellSysutil->AddFunc(0xed5d96af, cellAudioOutGetConfiguration);
	cellSysutil->AddFunc(0xc96e89e9, cellAudioOutSetCopyControl);

	cellSysutil->AddFunc(0xa11552f6, cellSysutilGetBgmPlaybackStatus);
	cellSysutil->AddFunc(0x6cfd856f, cellSysutilGetBgmPlaybackStatus2);
	cellSysutil->AddFunc(0x220894e3, cellSysutilEnableBgmPlayback);
	cellSysutil->AddFunc(0xac58ad2b, cellSysutilEnableBgmPlaybackEx);
	cellSysutil->AddFunc(0xcfdd8e87, cellSysutilDisableBgmPlayback);
	cellSysutil->AddFunc(0xa36335a5, cellSysutilDisableBgmPlaybackEx);

	cellSysutil->AddFunc(0x1e7bff94, cellSysCacheMount);
	cellSysutil->AddFunc(0x744c1544, cellSysCacheClear);

	cellSysutil->AddFunc(0x9117df20, cellHddGameCheck);
	//cellSysutil->AddFunc(0x4bdec82a, cellHddGameCheck2);
	//cellSysutil->AddFunc(0xf82e2ef7, cellHddGameGetSizeKB);
	//cellSysutil->AddFunc(0x9ca9ffa7, cellHddGameSetSystemVer);
	//cellSysutil->AddFunc(0xafd605b3, cellHddGameExitBroken);

	//cellSysutil_SaveData
	//cellSysutil->AddFunc(0x04c06fc2, cellSaveDataGetListItem);
	//cellSysutil->AddFunc(0x273d116a, cellSaveDataUserListExport);
	//cellSysutil->AddFunc(0x27cb8bc2, cellSaveDataListDelete);
	//cellSysutil->AddFunc(0x39d6ee43, cellSaveDataUserListImport);
	//cellSysutil->AddFunc(0x46a2d878, cellSaveDataFixedExport);
	//cellSysutil->AddFunc(0x491cc554, cellSaveDataListExport);
	//cellSysutil->AddFunc(0x52541151, cellSaveDataFixedImport);
	//cellSysutil->AddFunc(0x529231b0, cellSaveDataUserFixedImport);
	//cellSysutil->AddFunc(0x6b4e0de6, cellSaveDataListImport);
	//cellSysutil->AddFunc(0x7048a9ba, cellSaveDataUserListDelete);
	//cellSysutil->AddFunc(0x95ae2cde, cellSaveDataUserFixedExport);
	//cellSysutil->AddFunc(0xf6482036, cellSaveDataUserGetListItem);
	cellSysutil->AddFunc(0x2de0d663, cellSaveDataListSave2);
	cellSysutil->AddFunc(0x1dfbfdd6, cellSaveDataListLoad2);
	cellSysutil->AddFunc(0x2aae9ef5, cellSaveDataFixedSave2);
	cellSysutil->AddFunc(0x2a8eada2, cellSaveDataFixedLoad2);
	cellSysutil->AddFunc(0x8b7ed64b, cellSaveDataAutoSave2);
	cellSysutil->AddFunc(0xfbd5c856, cellSaveDataAutoLoad2);
	//cellSysutil->AddFunc(0x4dd03a4e, cellSaveDataListAutoSave);
	//cellSysutil->AddFunc(0x21425307, cellSaveDataListAutoLoad);
	//cellSysutil->AddFunc(0xedadd797, cellSaveDataDelete2);
	//cellSysutil->AddFunc(0x0f03cfb0, cellSaveDataUserListSave);
	//cellSysutil->AddFunc(0x39dd8425, cellSaveDataUserListLoad);
	//cellSysutil->AddFunc(0x40b34847, cellSaveDataUserFixedSave);
	//cellSysutil->AddFunc(0x6e7264ed, cellSaveDataUserFixedLoad);
	//cellSysutil->AddFunc(0x52aac4fa, cellSaveDataUserAutoSave);
	//cellSysutil->AddFunc(0xcdc6aefd, cellSaveDataUserAutoLoad);
	//cellSysutil->AddFunc(0x0e091c36, cellSaveDataUserListAutoSave);
	//cellSysutil->AddFunc(0xe7fa820b, cellSaveDataEnableOverlay);

	cellSysutil->AddFunc(0x6d087930, cellWebBrowserEstimate2);

	cellSysutil->AddFunc(0xe7951dee, cellGameDataCheckCreate);
	cellSysutil->AddFunc(0xc9645c41, cellGameDataCheckCreate2);
}
