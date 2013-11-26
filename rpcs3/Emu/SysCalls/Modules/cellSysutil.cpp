#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/SC_FUNC.h"

enum
{
	CELL_SYSUTIL_SYSTEMPARAM_ID_LANG							= 0x0111,
	CELL_SYSUTIL_SYSTEMPARAM_ID_ENTER_BUTTON_ASSIGN				= 0x0112,
	CELL_SYSUTIL_SYSTEMPARAM_ID_DATE_FORMAT						= 0x0114,
	CELL_SYSUTIL_SYSTEMPARAM_ID_TIME_FORMAT						= 0x0115,
	CELL_SYSUTIL_SYSTEMPARAM_ID_TIMEZONE						= 0x0116,
	CELL_SYSUTIL_SYSTEMPARAM_ID_SUMMERTIME						= 0x0117,
	CELL_SYSUTIL_SYSTEMPARAM_ID_GAME_PARENTAL_LEVEL				= 0x0121,
	CELL_SYSUTIL_SYSTEMPARAM_ID_GAME_PARENTAL_LEVEL0_RESTRICT	= 0x0123,
	CELL_SYSUTIL_SYSTEMPARAM_ID_CURRENT_USER_HAS_NP_ACCOUNT		= 0x0141,
	CELL_SYSUTIL_SYSTEMPARAM_ID_CAMERA_PLFREQ					= 0x0151,
	CELL_SYSUTIL_SYSTEMPARAM_ID_PAD_RUMBLE						= 0x0152,
	CELL_SYSUTIL_SYSTEMPARAM_ID_KEYBOARD_TYPE					= 0x0153,
	CELL_SYSUTIL_SYSTEMPARAM_ID_JAPANESE_KEYBOARD_ENTRY_METHOD	= 0x0154,
	CELL_SYSUTIL_SYSTEMPARAM_ID_CHINESE_KEYBOARD_ENTRY_METHOD	= 0x0155,
	CELL_SYSUTIL_SYSTEMPARAM_ID_PAD_AUTOOFF						= 0x0156,
};

enum
{
	CELL_SYSUTIL_LANG_JAPANESE		= 0,
	CELL_SYSUTIL_LANG_ENGLISH_US	= 1,
	CELL_SYSUTIL_LANG_FRENCH		= 2,
	CELL_SYSUTIL_LANG_SPANISH		= 3,
	CELL_SYSUTIL_LANG_GERMAN		= 4,
	CELL_SYSUTIL_LANG_ITALIAN		= 5,
	CELL_SYSUTIL_LANG_DUTCH			= 6,
	CELL_SYSUTIL_LANG_PORTUGUESE_PT	= 7,
	CELL_SYSUTIL_LANG_RUSSIAN		= 8,
	CELL_SYSUTIL_LANG_KOREAN		= 9,
	CELL_SYSUTIL_LANG_CHINESE_T		= 10,
	CELL_SYSUTIL_LANG_CHINESE_S		= 11,
	CELL_SYSUTIL_LANG_FINNISH		= 12,
	CELL_SYSUTIL_LANG_SWEDISH		= 13,
	CELL_SYSUTIL_LANG_DANISH		= 14,
	CELL_SYSUTIL_LANG_NORWEGIAN		= 15,
	CELL_SYSUTIL_LANG_POLISH		= 16,
	CELL_SYSUTIL_LANG_PORTUGUESE_BR	= 17,
	CELL_SYSUTIL_LANG_ENGLISH_GB	= 18,
};

enum
{
	CELL_SYSUTIL_ENTER_BUTTON_ASSIGN_CIRCLE = 0,
	CELL_SYSUTIL_ENTER_BUTTON_ASSIGN_CROSS	= 1,
};

enum
{
	CELL_SYSUTIL_DATE_FMT_YYYYMMDD = 0,
	CELL_SYSUTIL_DATE_FMT_DDMMYYYY = 1,
	CELL_SYSUTIL_DATE_FMT_MMDDYYYY = 2,
};

enum
{
	CELL_SYSUTIL_TIME_FMT_CLOCK12 = 0,
	CELL_SYSUTIL_TIME_FMT_CLOCK24 = 1,
};

enum
{
	CELL_SYSUTIL_GAME_PARENTAL_OFF		= 0,
	CELL_SYSUTIL_GAME_PARENTAL_LEVEL01	= 1,
	CELL_SYSUTIL_GAME_PARENTAL_LEVEL02	= 2,
	CELL_SYSUTIL_GAME_PARENTAL_LEVEL03	= 3,
	CELL_SYSUTIL_GAME_PARENTAL_LEVEL04	= 4,
	CELL_SYSUTIL_GAME_PARENTAL_LEVEL05	= 5,
	CELL_SYSUTIL_GAME_PARENTAL_LEVEL06	= 6,
	CELL_SYSUTIL_GAME_PARENTAL_LEVEL07	= 7,
	CELL_SYSUTIL_GAME_PARENTAL_LEVEL08	= 8,
	CELL_SYSUTIL_GAME_PARENTAL_LEVEL09	= 9,
	CELL_SYSUTIL_GAME_PARENTAL_LEVEL10	= 10,
	CELL_SYSUTIL_GAME_PARENTAL_LEVEL11	= 11,
};

enum
{
	CELL_SYSUTIL_GAME_PARENTAL_LEVEL0_RESTRICT_OFF	= 0,
	CELL_SYSUTIL_GAME_PARENTAL_LEVEL0_RESTRICT_ON	= 1,
};

enum
{
	CELL_SYSUTIL_CAMERA_PLFREQ_DISABLED			= 0,
	CELL_SYSUTIL_CAMERA_PLFREQ_50HZ				= 1,
	CELL_SYSUTIL_CAMERA_PLFREQ_60HZ				= 2,
	CELL_SYSUTIL_CAMERA_PLFREQ_DEVCIE_DEPEND	= 4,
};

enum
{
	CELL_SYSUTIL_PAD_RUMBLE_OFF	= 0,
	CELL_SYSUTIL_PAD_RUMBLE_ON	= 1,
};

void cellSysutil_init();
Module cellSysutil(0x0015, cellSysutil_init);


enum
{
	CELL_MSGDIALOG_BUTTON_NONE		= -1,
	CELL_MSGDIALOG_BUTTON_INVALID	= 0,
	CELL_MSGDIALOG_BUTTON_OK		= 1,
	CELL_MSGDIALOG_BUTTON_YES		= 1,
	CELL_MSGDIALOG_BUTTON_NO		= 2,
	CELL_MSGDIALOG_BUTTON_ESCAPE	= 3,
};

enum CellMsgDialogType
{
	CELL_MSGDIALOG_DIALOG_TYPE_ERROR	= 0x00000000,
	CELL_MSGDIALOG_DIALOG_TYPE_NORMAL	= 0x00000001,

	CELL_MSGDIALOG_BUTTON_TYPE_NONE		= 0x00000000,
	CELL_MSGDIALOG_BUTTON_TYPE_YESNO	= 0x00000010,

	CELL_MSGDIALOG_DEFAULT_CURSOR_YES	= 0x00000000,
	CELL_MSGDIALOG_DEFAULT_CURSOR_NO	= 0x00000100,
};

int cellSysutilGetSystemParamInt(int id, mem32_t value)
{
	cellSysutil.Log("cellSysutilGetSystemParamInt(id=0x%x, value_addr=0x%x)", id, value.GetAddr());

	if(!value.IsGood())
	{
		return CELL_EFAULT;
	}

	switch(id)
	{
	case CELL_SYSUTIL_SYSTEMPARAM_ID_LANG:
		cellSysutil.Warning("cellSysutilGetSystemParamInt: CELL_SYSUTIL_SYSTEMPARAM_ID_LANG");
		value = CELL_SYSUTIL_LANG_ENGLISH_US;
	break;

	case CELL_SYSUTIL_SYSTEMPARAM_ID_ENTER_BUTTON_ASSIGN:
		cellSysutil.Warning("cellSysutilGetSystemParamInt: CELL_SYSUTIL_SYSTEMPARAM_ID_ENTER_BUTTON_ASSIGN");
		value = CELL_SYSUTIL_ENTER_BUTTON_ASSIGN_CROSS;
	break;

	case CELL_SYSUTIL_SYSTEMPARAM_ID_DATE_FORMAT:
		cellSysutil.Warning("cellSysutilGetSystemParamInt: CELL_SYSUTIL_SYSTEMPARAM_ID_DATE_FORMAT");
		value = CELL_SYSUTIL_DATE_FMT_DDMMYYYY;
	break;

	case CELL_SYSUTIL_SYSTEMPARAM_ID_TIME_FORMAT:
		cellSysutil.Warning("cellSysutilGetSystemParamInt: CELL_SYSUTIL_SYSTEMPARAM_ID_TIME_FORMAT");
		value = CELL_SYSUTIL_TIME_FMT_CLOCK24;
	break;

	case CELL_SYSUTIL_SYSTEMPARAM_ID_TIMEZONE:
		cellSysutil.Warning("cellSysutilGetSystemParamInt: CELL_SYSUTIL_SYSTEMPARAM_ID_TIMEZONE");
		value = 3;
	break;

	case CELL_SYSUTIL_SYSTEMPARAM_ID_SUMMERTIME:
		cellSysutil.Warning("cellSysutilGetSystemParamInt: CELL_SYSUTIL_SYSTEMPARAM_ID_SUMMERTIME");
		value = 1;
	break;

	case CELL_SYSUTIL_SYSTEMPARAM_ID_GAME_PARENTAL_LEVEL:
		cellSysutil.Warning("cellSysutilGetSystemParamInt: CELL_SYSUTIL_SYSTEMPARAM_ID_GAME_PARENTAL_LEVEL");
		value = CELL_SYSUTIL_GAME_PARENTAL_OFF;
	break;

	case CELL_SYSUTIL_SYSTEMPARAM_ID_GAME_PARENTAL_LEVEL0_RESTRICT:
		cellSysutil.Warning("cellSysutilGetSystemParamInt: CELL_SYSUTIL_SYSTEMPARAM_ID_GAME_PARENTAL_LEVEL0_RESTRICT");
		value = CELL_SYSUTIL_GAME_PARENTAL_LEVEL0_RESTRICT_OFF;
	break;

	case CELL_SYSUTIL_SYSTEMPARAM_ID_CURRENT_USER_HAS_NP_ACCOUNT:
		cellSysutil.Warning("cellSysutilGetSystemParamInt: CELL_SYSUTIL_SYSTEMPARAM_ID_CURRENT_USER_HAS_NP_ACCOUNT");
		value = 0;
	break;

	case CELL_SYSUTIL_SYSTEMPARAM_ID_CAMERA_PLFREQ:
		cellSysutil.Warning("cellSysutilGetSystemParamInt: CELL_SYSUTIL_SYSTEMPARAM_ID_CAMERA_PLFREQ");
		value = CELL_SYSUTIL_CAMERA_PLFREQ_DISABLED;
	break;

	case CELL_SYSUTIL_SYSTEMPARAM_ID_PAD_RUMBLE:
		cellSysutil.Warning("cellSysutilGetSystemParamInt: CELL_SYSUTIL_SYSTEMPARAM_ID_PAD_RUMBLE");
		value = CELL_SYSUTIL_PAD_RUMBLE_OFF;
	break;

	case CELL_SYSUTIL_SYSTEMPARAM_ID_KEYBOARD_TYPE:
		cellSysutil.Warning("cellSysutilGetSystemParamInt: CELL_SYSUTIL_SYSTEMPARAM_ID_KEYBOARD_TYPE");
		value = 0;
	break;

	case CELL_SYSUTIL_SYSTEMPARAM_ID_JAPANESE_KEYBOARD_ENTRY_METHOD:
		cellSysutil.Warning("cellSysutilGetSystemParamInt: CELL_SYSUTIL_SYSTEMPARAM_ID_JAPANESE_KEYBOARD_ENTRY_METHOD");
		value = 0;
	break;

	case CELL_SYSUTIL_SYSTEMPARAM_ID_CHINESE_KEYBOARD_ENTRY_METHOD:
		cellSysutil.Warning("cellSysutilGetSystemParamInt: CELL_SYSUTIL_SYSTEMPARAM_ID_CHINESE_KEYBOARD_ENTRY_METHOD");
		value = 0;
	break;

	case CELL_SYSUTIL_SYSTEMPARAM_ID_PAD_AUTOOFF:
		cellSysutil.Warning("cellSysutilGetSystemParamInt: CELL_SYSUTIL_SYSTEMPARAM_ID_PAD_AUTOOFF");
		value = 0;
	break;

	default:
		return CELL_EINVAL;
	}

	return CELL_OK;
}

int cellVideoOutGetState(u32 videoOut, u32 deviceIndex, u32 state_addr)
{
	cellSysutil.Log("cellVideoOutGetState(videoOut=0x%x, deviceIndex=0x%x, state_addr=0x%x)", videoOut, deviceIndex, state_addr);

	if(deviceIndex) return CELL_VIDEO_OUT_ERROR_DEVICE_NOT_FOUND;

	CellVideoOutState state;
	memset(&state, 0, sizeof(CellVideoOutState));

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

int cellVideoOutGetResolution(u32 resolutionId, u32 resolution_addr)
{
	cellSysutil.Log("cellVideoOutGetResolution(resolutionId=%d, resolution_addr=0x%x)",
		resolutionId, resolution_addr);

	if(!Memory.IsGoodAddr(resolution_addr, sizeof(CellVideoOutResolution)))
	{
		return CELL_EFAULT;
	}
	
	u32 num = ResolutionIdToNum(resolutionId);

	if(!num)
	{
		return CELL_EINVAL;
	}

	CellVideoOutResolution res;
	re(res.width, ResolutionTable[num].width);
	re(res.height, ResolutionTable[num].height);

	Memory.WriteData(resolution_addr, res);

	return CELL_VIDEO_OUT_SUCCEEDED;
}

int cellVideoOutConfigure(u32 videoOut, u32 config_addr, u32 option_addr, u32 waitForEvent)
{
	cellSysutil.Warning("cellVideoOutConfigure(videoOut=%d, config_addr=0x%x, option_addr=0x%x, waitForEvent=0x%x)",
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
	cellSysutil.Warning("cellVideoOutGetConfiguration(videoOut=%d, config_addr=0x%x, option_addr=0x%x)",
		videoOut, config_addr, option_addr);

	if(!Memory.IsGoodAddr(config_addr, sizeof(CellVideoOutConfiguration))) return CELL_EFAULT;

	CellVideoOutConfiguration config;
	memset(&config, 0, sizeof(CellVideoOutConfiguration));

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
	cellSysutil.Error("Unimplemented function: cellVideoOutGetDeviceInfo(videoOut=%u, deviceIndex=%u, info_addr=0x%x)",
		videoOut, deviceIndex, info.GetAddr());

	if(deviceIndex) return CELL_VIDEO_OUT_ERROR_DEVICE_NOT_FOUND;

	info->portType = CELL_VIDEO_OUT_PORT_HDMI;
	info->colorSpace = Emu.GetGSManager().GetColorSpace();
	//info->latency = ;
	//info->availableModeCount = ;
	info->state = CELL_VIDEO_OUT_DEVICE_STATE_AVAILABLE;
	//info->rgbOutputRange = ;
	
	return CELL_OK;
}

int cellVideoOutGetNumberOfDevice(u32 videoOut)
{
	cellSysutil.Warning("cellVideoOutGetNumberOfDevice(videoOut=%d)", videoOut);

	switch(videoOut)
	{
	case CELL_VIDEO_OUT_PRIMARY: return 1;
	case CELL_VIDEO_OUT_SECONDARY: return 0;
	}

	return CELL_VIDEO_OUT_ERROR_UNSUPPORTED_VIDEO_OUT;
}

int cellVideoOutGetResolutionAvailability(u32 videoOut, u32 resolutionId, u32 aspect, u32 option)
{
	cellSysutil.Warning("cellVideoOutGetResolutionAvailability(videoOut=%d, resolutionId=0x%x, option_addr=0x%x, aspect=0x%x, option=0x%x)",
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
	//cellSysutil.Warning("cellSysutilCheckCallback()");
	Emu.GetCallbackManager().m_exit_callback.Check();

	return CELL_OK;
}

int cellSysutilRegisterCallback(int slot, u64 func_addr, u64 userdata)
{
	cellSysutil.Warning("cellSysutilRegisterCallback(slot=%d, func_addr=0x%llx, userdata=0x%llx)", slot, func_addr, userdata);
	Emu.GetCallbackManager().m_exit_callback.Register(slot, func_addr, userdata);

	wxGetApp().SendDbgCommand(DID_REGISTRED_CALLBACK);

	return CELL_OK;
}

int cellSysutilUnregisterCallback(int slot)
{
	cellSysutil.Warning("cellSysutilUnregisterCallback(slot=%d)", slot);
	Emu.GetCallbackManager().m_exit_callback.Unregister(slot);

	wxGetApp().SendDbgCommand(DID_UNREGISTRED_CALLBACK);

	return CELL_OK;
}

int cellMsgDialogOpen2(u32 type, u32 msgString_addr, u32 callback_addr, u32 userData, u32 extParam)
{
	long style = 0;

	if(type & CELL_MSGDIALOG_DIALOG_TYPE_NORMAL)
	{
		style |= wxICON_EXCLAMATION;
	}
	else
	{
		style |= wxICON_ERROR;
	}

	if(type & CELL_MSGDIALOG_BUTTON_TYPE_YESNO)
	{
		style |= wxYES_NO;
	}
	else
	{
		style |= wxOK;
	}

	int res = wxMessageBox(Memory.ReadString(msgString_addr), wxGetApp().GetAppName(), style);

	u64 status;

	switch(res)
	{
	case wxOK: status = CELL_MSGDIALOG_BUTTON_OK; break;
	case wxYES: status = CELL_MSGDIALOG_BUTTON_YES; break;
	case wxNO: status = CELL_MSGDIALOG_BUTTON_NO; break;

	default:
		if(res)
		{
			status = CELL_MSGDIALOG_BUTTON_INVALID;
			break;
		}

		status = CELL_MSGDIALOG_BUTTON_NONE;
	break;
	}

	Callback2 callback(0, callback_addr, userData);
	callback.Handle(status);
	callback.Branch(true);

	return CELL_OK;
}

void cellSysutil_init()
{
	cellSysutil.AddFunc(0x40e895d3, cellSysutilGetSystemParamInt);

	cellSysutil.AddFunc(0x887572d5, cellVideoOutGetState);
	cellSysutil.AddFunc(0xe558748d, cellVideoOutGetResolution);
	cellSysutil.AddFunc(0x0bae8772, cellVideoOutConfigure);
	cellSysutil.AddFunc(0x15b0b0cd, cellVideoOutGetConfiguration);
	cellSysutil.AddFunc(0x1e930eef, cellVideoOutGetDeviceInfo);
	cellSysutil.AddFunc(0x75bbb672, cellVideoOutGetNumberOfDevice);
	cellSysutil.AddFunc(0xa322db75, cellVideoOutGetResolutionAvailability);

	cellSysutil.AddFunc(0x189a74da, cellSysutilCheckCallback);
	cellSysutil.AddFunc(0x9d98afa0, cellSysutilRegisterCallback);
	cellSysutil.AddFunc(0x02ff3c1b, cellSysutilUnregisterCallback);

	cellSysutil.AddFunc(0x7603d3db, cellMsgDialogOpen2);
}