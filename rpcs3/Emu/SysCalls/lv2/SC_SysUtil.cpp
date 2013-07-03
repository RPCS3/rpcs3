#include "stdafx.h"
#include "Emu/GS/sysutil_video.h"
#include "Emu/SysCalls/SysCalls.h"

//cellVideo
SysCallBase sc_v("cellVideo");

int cellVideoOutGetState(u32 videoOut, u32 deviceIndex, u32 state_addr)
{
	sc_v.Log("cellVideoOutGetState(videoOut=0x%x, deviceIndex=0x%x, state_addr=0x%x)", videoOut, deviceIndex, state_addr);

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
	sc_v.Log("cellVideoOutGetResolution(resolutionId=%d, resolution_addr=0x%x)",
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
	sc_v.Warning("cellVideoOutConfigure(videoOut=%d, config_addr=0x%x, option_addr=0x%x, waitForEvent=0x%x)",
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
	sc_v.Warning("cellVideoOutGetConfiguration(videoOut=%d, config_addr=0x%x, option_addr=0x%x)",
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

int cellVideoOutGetNumberOfDevice(u32 videoOut)
{
	sc_v.Warning("cellVideoOutGetNumberOfDevice(videoOut=%d)", videoOut);

	switch(videoOut)
	{
	case CELL_VIDEO_OUT_PRIMARY: return 1;
	case CELL_VIDEO_OUT_SECONDARY: return 0;
	}

	return CELL_VIDEO_OUT_ERROR_UNSUPPORTED_VIDEO_OUT;
}

int cellVideoOutGetResolutionAvailability(u32 videoOut, u32 resolutionId, u32 aspect, u32 option)
{
	sc_v.Warning("cellVideoOutGetResolutionAvailability(videoOut=%d, resolutionId=0x%x, option_addr=0x%x, aspect=0x%x, option=0x%x)",
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

SysCallBase sc_sysutil("cellSysutil");

int cellSysutilCheckCallback()
{
	//sc_sysutil.Warning("cellSysutilCheckCallback()");
	Emu.GetCallbackManager().m_exit_callback.Check();

	return CELL_OK;
}

int cellSysutilRegisterCallback(int slot, u64 func_addr, u64 userdata)
{
	sc_sysutil.Warning("cellSysutilRegisterCallback(slot=%d, func_addr=0x%llx, userdata=0x%llx)", slot, func_addr, userdata);
	Emu.GetCallbackManager().m_exit_callback.Register(slot, func_addr, userdata);

	wxGetApp().SendDbgCommand(DID_REGISTRED_CALLBACK);

	return CELL_OK;
}

int cellSysutilUnregisterCallback(int slot)
{
	sc_sysutil.Warning("cellSysutilUnregisterCallback(slot=%d)", slot);
	Emu.GetCallbackManager().m_exit_callback.Unregister(slot);

	wxGetApp().SendDbgCommand(DID_UNREGISTRED_CALLBACK);

	return CELL_OK;
}