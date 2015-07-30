#include "stdafx.h"
#include "Utilities/Log.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/SysCalls/Modules.h"
#include "Emu/SysCalls/Callback.h"

#include "Emu/DbgCommand.h"
#include "rpcs3/Ini.h"
#include "Emu/FS/vfsFile.h"
#include "Loader/PSF.h"
#include "Emu/RSX/sysutil_video.h"
#include "Emu/RSX/GSManager.h"
#include "Emu/Audio/AudioManager.h"
#include "Emu/FS/VFS.h"
#include "cellMsgDialog.h"
#include "cellAudioIn.h"
#include "cellAudioOut.h"
#include "cellSysutil.h"

extern Module cellSysutil;

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

s32 cellVideoOutGetState(u32 videoOut, u32 deviceIndex, vm::ptr<CellVideoOutState> state)
{
	cellSysutil.Log("cellVideoOutGetState(videoOut=%d, deviceIndex=%d, state=*0x%x)", videoOut, deviceIndex, state);

	if(deviceIndex) return CELL_VIDEO_OUT_ERROR_DEVICE_NOT_FOUND;

	switch(videoOut)
	{
	case CELL_VIDEO_OUT_PRIMARY:
		state->state = Emu.GetGSManager().GetState();
		state->colorSpace = Emu.GetGSManager().GetColorSpace();
		state->displayMode.resolutionId = Emu.GetGSManager().GetInfo().mode.resolutionId;
		state->displayMode.scanMode = Emu.GetGSManager().GetInfo().mode.scanMode;
		state->displayMode.conversion = Emu.GetGSManager().GetInfo().mode.conversion;
		state->displayMode.aspect = Emu.GetGSManager().GetInfo().mode.aspect;
		state->displayMode.refreshRates = Emu.GetGSManager().GetInfo().mode.refreshRates;
		return CELL_VIDEO_OUT_SUCCEEDED;
		
	case CELL_VIDEO_OUT_SECONDARY:
		*state = { CELL_VIDEO_OUT_OUTPUT_STATE_DISABLED }; // ???
		return CELL_VIDEO_OUT_SUCCEEDED;
	}

	return CELL_VIDEO_OUT_ERROR_UNSUPPORTED_VIDEO_OUT;
}

s32 cellVideoOutGetResolution(u32 resolutionId, vm::ptr<CellVideoOutResolution> resolution)
{
	cellSysutil.Log("cellVideoOutGetResolution(resolutionId=%d, resolution=*0x%x)", resolutionId, resolution);

	u32 num = ResolutionIdToNum(resolutionId);
	if(!num)
		return CELL_EINVAL;

	resolution->width = ResolutionTable[num].width;
	resolution->height = ResolutionTable[num].height;

	return CELL_VIDEO_OUT_SUCCEEDED;
}

s32 cellVideoOutConfigure(u32 videoOut, vm::ptr<CellVideoOutConfiguration> config, vm::ptr<CellVideoOutOption> option, u32 waitForEvent)
{
	cellSysutil.Warning("cellVideoOutConfigure(videoOut=%d, config=*0x%x, option=*0x%x, waitForEvent=0x%x)", videoOut, config, option, waitForEvent);

	switch(videoOut)
	{
	case CELL_VIDEO_OUT_PRIMARY:
		if(config->resolutionId)
		{
			Emu.GetGSManager().GetInfo().mode.resolutionId = config->resolutionId;
		}

		Emu.GetGSManager().GetInfo().mode.format = config->format;

		if(config->aspect)
		{
			Emu.GetGSManager().GetInfo().mode.aspect = config->aspect;
		}

		if(config->pitch)
		{
			Emu.GetGSManager().GetInfo().mode.pitch = config->pitch;
		}

		return CELL_VIDEO_OUT_SUCCEEDED;

	case CELL_VIDEO_OUT_SECONDARY:
		return CELL_VIDEO_OUT_SUCCEEDED;
	}

	return CELL_VIDEO_OUT_ERROR_UNSUPPORTED_VIDEO_OUT;
}

s32 cellVideoOutGetConfiguration(u32 videoOut, vm::ptr<CellVideoOutConfiguration> config, vm::ptr<CellVideoOutOption> option)
{
	cellSysutil.Warning("cellVideoOutGetConfiguration(videoOut=%d, config=*0x%x, option=*0x%x)", videoOut, config, option);

	if (option) *option = {};
	*config = {};

	switch(videoOut)
	{
	case CELL_VIDEO_OUT_PRIMARY:
		config->resolutionId = Emu.GetGSManager().GetInfo().mode.resolutionId;
		config->format = Emu.GetGSManager().GetInfo().mode.format;
		config->aspect = Emu.GetGSManager().GetInfo().mode.aspect;
		config->pitch = Emu.GetGSManager().GetInfo().mode.pitch;

		return CELL_VIDEO_OUT_SUCCEEDED;

	case CELL_VIDEO_OUT_SECONDARY:

		return CELL_VIDEO_OUT_SUCCEEDED;
	}

	return CELL_VIDEO_OUT_ERROR_UNSUPPORTED_VIDEO_OUT;
}

s32 cellVideoOutGetDeviceInfo(u32 videoOut, u32 deviceIndex, vm::ptr<CellVideoOutDeviceInfo> info)
{
	cellSysutil.Warning("cellVideoOutGetDeviceInfo(videoOut=%d, deviceIndex=%d, info=*0x%x)", videoOut, deviceIndex, info);

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

s32 cellVideoOutGetNumberOfDevice(u32 videoOut)
{
	cellSysutil.Warning("cellVideoOutGetNumberOfDevice(videoOut=%d)", videoOut);

	switch(videoOut)
	{
	case CELL_VIDEO_OUT_PRIMARY: return 1;
	case CELL_VIDEO_OUT_SECONDARY: return 0;
	}

	return CELL_VIDEO_OUT_ERROR_UNSUPPORTED_VIDEO_OUT;
}

s32 cellVideoOutGetResolutionAvailability(u32 videoOut, u32 resolutionId, u32 aspect, u32 option)
{
	cellSysutil.Warning("cellVideoOutGetResolutionAvailability(videoOut=%d, resolutionId=0x%x, aspect=%d, option=%d)", videoOut, resolutionId, aspect, option);

	if (!Ini.GS3DTV.GetValue() && (resolutionId == CELL_VIDEO_OUT_RESOLUTION_720_3D_FRAME_PACKING || resolutionId == CELL_VIDEO_OUT_RESOLUTION_1024x720_3D_FRAME_PACKING ||
								  resolutionId == CELL_VIDEO_OUT_RESOLUTION_960x720_3D_FRAME_PACKING || resolutionId == CELL_VIDEO_OUT_RESOLUTION_800x720_3D_FRAME_PACKING ||
								  resolutionId == CELL_VIDEO_OUT_RESOLUTION_640x720_3D_FRAME_PACKING))
		return 0;

	switch(videoOut)
	{
	case CELL_VIDEO_OUT_PRIMARY: return 1;
	case CELL_VIDEO_OUT_SECONDARY: return 0;
	}

	return CELL_VIDEO_OUT_ERROR_UNSUPPORTED_VIDEO_OUT;
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

s32 cellAudioOutGetSoundAvailability(u32 audioOut, u32 type, u32 fs, u32 option)
{
	cellSysutil.Warning("cellAudioOutGetSoundAvailability(audioOut=%d, type=%d, fs=0x%x, option=%d)", audioOut, type, fs, option);

	option = 0;

	s32 available = 8; // should be at least 2

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

s32 cellAudioOutGetSoundAvailability2(u32 audioOut, u32 type, u32 fs, u32 ch, u32 option)
{
	cellSysutil.Warning("cellAudioOutGetSoundAvailability2(audioOut=%d, type=%d, fs=0x%x, ch=%d, option=%d)", audioOut, type, fs, ch, option);

	option = 0;

	s32 available = 8; // should be at least 2

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

s32 cellAudioOutGetState(u32 audioOut, u32 deviceIndex, vm::ptr<CellAudioOutState> state)
{
	cellSysutil.Warning("cellAudioOutGetState(audioOut=0x%x, deviceIndex=0x%x, state=*0x%x)", audioOut, deviceIndex, state);

	*state = {};

	switch(audioOut)
	{
	case CELL_AUDIO_OUT_PRIMARY:
		state->state = CELL_AUDIO_OUT_OUTPUT_STATE_ENABLED;
		state->encoder = CELL_AUDIO_OUT_CODING_TYPE_LPCM;
		state->downMixer = CELL_AUDIO_OUT_DOWNMIXER_NONE;
		state->soundMode.type = CELL_AUDIO_OUT_CODING_TYPE_LPCM;
		state->soundMode.channel = CELL_AUDIO_OUT_CHNUM_8;
		state->soundMode.fs = CELL_AUDIO_OUT_FS_48KHZ;
		state->soundMode.reserved = 0;
		state->soundMode.layout = CELL_AUDIO_OUT_SPEAKER_LAYOUT_8CH_LREClrxy;

		return CELL_OK;

	case CELL_AUDIO_OUT_SECONDARY:
		state->state = CELL_AUDIO_OUT_OUTPUT_STATE_DISABLED;

		return CELL_OK;
	}

	return CELL_AUDIO_OUT_ERROR_UNSUPPORTED_AUDIO_OUT;
}

s32 cellAudioOutConfigure(u32 audioOut, vm::ptr<CellAudioOutConfiguration> config, vm::ptr<CellAudioOutOption> option, u32 waitForEvent)
{
	cellSysutil.Warning("cellAudioOutConfigure(audioOut=%d, config=*0x%x, option=*0x%x, waitForEvent=%d)", audioOut, config, option, waitForEvent);

	switch(audioOut)
	{
	case CELL_AUDIO_OUT_PRIMARY:
		if (config->channel)
		{
			//Emu.GetAudioManager().GetInfo().mode.channel = config->channel;
		}

		//Emu.GetAudioManager().GetInfo().mode.encoder = config->encoder;

		if(config->downMixer)
		{
			//Emu.GetAudioManager().GetInfo().mode.downMixer = config->downMixer;
		}

		return CELL_OK;

	case CELL_AUDIO_OUT_SECONDARY:
		return CELL_OK;
	}

	return CELL_AUDIO_OUT_ERROR_UNSUPPORTED_AUDIO_OUT;
}

s32 cellAudioOutGetConfiguration(u32 audioOut, vm::ptr<CellAudioOutConfiguration> config, vm::ptr<CellAudioOutOption> option)
{
	cellSysutil.Warning("cellAudioOutGetConfiguration(audioOut=%d, config=*0x%x, option=*0x%x)", audioOut, config, option);

	if (option) *option = {};
	*config = {};

	switch(audioOut)
	{
	case CELL_AUDIO_OUT_PRIMARY:
		config->channel = CELL_AUDIO_OUT_CHNUM_8;
		config->encoder = CELL_AUDIO_OUT_CODING_TYPE_LPCM;
		config->downMixer = CELL_AUDIO_OUT_DOWNMIXER_NONE;

		return CELL_OK;

	case CELL_AUDIO_OUT_SECONDARY:

		return CELL_OK;
	}

	return CELL_AUDIO_OUT_ERROR_UNSUPPORTED_AUDIO_OUT;
}

s32 cellAudioOutGetNumberOfDevice(u32 audioOut)
{
	cellSysutil.Warning("cellAudioOutGetNumberOfDevice(audioOut=%d)", audioOut);

	switch(audioOut)
	{
	case CELL_AUDIO_OUT_PRIMARY: return 1;
	case CELL_AUDIO_OUT_SECONDARY: return 0;
	}

	return CELL_AUDIO_OUT_ERROR_UNSUPPORTED_AUDIO_OUT;
}

s32 cellAudioOutGetDeviceInfo(u32 audioOut, u32 deviceIndex, vm::ptr<CellAudioOutDeviceInfo> info)
{
	cellSysutil.Todo("cellAudioOutGetDeviceInfo(audioOut=%d, deviceIndex=%d, info=*0x%x)", audioOut, deviceIndex, info);

	if(deviceIndex) return CELL_AUDIO_OUT_ERROR_DEVICE_NOT_FOUND;

	info->portType = CELL_AUDIO_OUT_PORT_HDMI;
	info->availableModeCount = 1;
	info->state = CELL_AUDIO_OUT_DEVICE_STATE_AVAILABLE;
	info->latency = 1000;
	info->availableModes[0].type = CELL_AUDIO_IN_CODING_TYPE_LPCM;
	info->availableModes[0].channel = CELL_AUDIO_OUT_CHNUM_8;
	info->availableModes[0].fs = CELL_AUDIO_OUT_FS_48KHZ;
	info->availableModes[0].layout = CELL_AUDIO_OUT_SPEAKER_LAYOUT_8CH_LREClrxy;

	return CELL_OK;
}

s32 cellAudioOutSetCopyControl(u32 audioOut, u32 control)
{
	cellSysutil.Warning("cellAudioOutSetCopyControl(audioOut=%d, control=%d)", audioOut, control);

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

	return CELL_OK;
}

struct CellSysCacheParam
{
	char cacheId[CELL_SYSCACHE_ID_SIZE];
	char getCachePath[CELL_SYSCACHE_PATH_MAX];
	vm::ptr<void> reserved;
};

s32 cellSysCacheClear(void)
{
	cellSysutil.Todo("cellSysCacheClear()");

	//if some software expects CELL_SYSCACHE_ERROR_NOTMOUNTED we need to check whether
	//it was mounted before, for that we would need to save the state which I don't know
	//where to put
	std::string localPath;
	Emu.GetVFS().GetDevice(std::string("/dev_hdd1/cache/"), localPath);

	return CELL_SYSCACHE_RET_OK_CLEARED;
}

s32 cellSysCacheMount(vm::ptr<CellSysCacheParam> param)
{
	cellSysutil.Warning("cellSysCacheMount(param=*0x%x)", param);

	//TODO: implement
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
	cellSysutil.Log("cellSysutilGetBgmPlaybackStatus(status=*0x%x)", status);

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
	cellSysutil.Log("cellSysutilGetBgmPlaybackStatus2(status2=*0x%x)", status2);

	// TODO
	status2->playerState = CELL_SYSUTIL_BGMPLAYBACK_STATUS_STOP;
	memset(status2->reserved, 0, sizeof(status2->reserved));

	return CELL_OK;
}

s32 cellWebBrowserEstimate2(vm::cptr<CellWebBrowserConfig2> config, vm::ptr<u32> memSize)
{
	cellSysutil.Warning("cellWebBrowserEstimate2(config=*0x%x, memSize=*0x%x)", config, memSize);

	// TODO: When cellWebBrowser stuff is implemented, change this to some real
	// needed memory buffer size.
	*memSize = 1 * 1024 * 1024; // 1 MB
	return CELL_OK;
}

extern void cellSysutil_SaveData_init();
extern void cellSysutil_GameData_init();
extern void cellSysutil_MsgDialog_init();
extern void cellSysutil_OskDialog_init();
extern void cellSysutil_Storage_init();

Module cellSysutil("cellSysutil", []()
{
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

	REG_FUNC(cellSysutil, cellSysutilGetSystemParamInt);
	REG_FUNC(cellSysutil, cellSysutilGetSystemParamString);

	REG_FUNC(cellSysutil, cellVideoOutGetState);
	REG_FUNC(cellSysutil, cellVideoOutGetResolution);
	REG_FUNC(cellSysutil, cellVideoOutConfigure);
	REG_FUNC(cellSysutil, cellVideoOutGetConfiguration);
	REG_FUNC(cellSysutil, cellVideoOutGetDeviceInfo);
	REG_FUNC(cellSysutil, cellVideoOutGetNumberOfDevice);
	REG_FUNC(cellSysutil, cellVideoOutGetResolutionAvailability);

	REG_FUNC(cellSysutil, cellSysutilCheckCallback);
	REG_FUNC(cellSysutil, cellSysutilRegisterCallback);
	REG_FUNC(cellSysutil, cellSysutilUnregisterCallback);

	REG_FUNC(cellSysutil, cellAudioOutGetState);
	REG_FUNC(cellSysutil, cellAudioOutConfigure);
	REG_FUNC(cellSysutil, cellAudioOutGetSoundAvailability);
	REG_FUNC(cellSysutil, cellAudioOutGetSoundAvailability2);
	REG_FUNC(cellSysutil, cellAudioOutGetDeviceInfo);
	REG_FUNC(cellSysutil, cellAudioOutGetNumberOfDevice);
	REG_FUNC(cellSysutil, cellAudioOutGetConfiguration);
	REG_FUNC(cellSysutil, cellAudioOutSetCopyControl);

	REG_FUNC(cellSysutil, cellSysutilGetBgmPlaybackStatus);
	REG_FUNC(cellSysutil, cellSysutilGetBgmPlaybackStatus2);
	REG_FUNC(cellSysutil, cellSysutilEnableBgmPlayback);
	REG_FUNC(cellSysutil, cellSysutilEnableBgmPlaybackEx);
	REG_FUNC(cellSysutil, cellSysutilDisableBgmPlayback);
	REG_FUNC(cellSysutil, cellSysutilDisableBgmPlaybackEx);

	REG_FUNC(cellSysutil, cellSysCacheMount);
	REG_FUNC(cellSysutil, cellSysCacheClear);

	//REG_FUNC(cellSysutil, cellSysutilRegisterCallbackDispatcher);
	//REG_FUNC(cellSysutil, cellSysutilPacketWrite);

	REG_FUNC(cellSysutil, cellWebBrowserEstimate2);
});
