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

	CellVideoOutResolution res = {0, 0};

	switch(resolutionId)
	{
	case CELL_VIDEO_OUT_RESOLUTION_1080: 
		res.width = re16(1920);
		res.height = re16(1080);
	break;

	case CELL_VIDEO_OUT_RESOLUTION_720:
		res.width = re16(1280);
		res.height = re16(720);
	break;

	case CELL_VIDEO_OUT_RESOLUTION_480:
		res.width = re16(720);
		res.height = re16(480);
	break;

	case CELL_VIDEO_OUT_RESOLUTION_576:
		res.width = re16(720);
		res.height = re16(576);
	break;

	case CELL_VIDEO_OUT_RESOLUTION_1600x1080:
		res.width = re16(1600);
		res.height = re16(1080);
	break;

	case CELL_VIDEO_OUT_RESOLUTION_1440x1080:
		res.width = re16(1440);
		res.height = re16(1080);
	break;

	case CELL_VIDEO_OUT_RESOLUTION_1280x1080:
		res.width = re16(1280);
		res.height = re16(1080);
	break;

	case CELL_VIDEO_OUT_RESOLUTION_960x1080:
		res.width = re16(960);
		res.height = re16(1080);
	break;

	default: return CELL_EINVAL;
	}

	Memory.WriteData(resolution_addr, res);
	return CELL_OK;
}