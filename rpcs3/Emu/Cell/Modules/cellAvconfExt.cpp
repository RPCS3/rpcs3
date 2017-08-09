#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/Cell/PPUModule.h"

#include "cellAudioIn.h"
#include "cellAudioOut.h"
#include "cellVideoOut.h"
#include "cellSysutil.h"

logs::channel cellAvconfExt("cellAvconfExt");

vm::gvar<f32> g_gamma; // TODO

s32 cellAudioOutUnregisterDevice(u32 deviceNumber)
{
	cellAvconfExt.todo("cellAudioOutUnregisterDevice(deviceNumber=0x%x)", deviceNumber);
	return CELL_OK;
}

s32 cellAudioOutGetDeviceInfo2(u32 deviceNumber, u32 deviceIndex, vm::ptr<CellAudioOutDeviceInfo2> info)
{
	cellAvconfExt.todo("cellAudioOutGetDeviceInfo2(deviceNumber=0x%x, deviceIndex=0x%x, info=*0x%x)", deviceNumber, deviceIndex, info);
	return CELL_OK;
}

s32 cellVideoOutSetXVColor()
{
	UNIMPLEMENTED_FUNC(cellAvconfExt);
	return CELL_OK;
}

s32 cellVideoOutSetupDisplay()
{
	UNIMPLEMENTED_FUNC(cellAvconfExt);
	return CELL_OK;
}

s32 cellAudioInGetDeviceInfo(u32 deviceNumber, u32 deviceIndex, vm::ptr<CellAudioInDeviceInfo> info)
{
	cellAvconfExt.todo("cellAudioInGetDeviceInfo(deviceNumber=0x%x, deviceIndex=0x%x, info=*0x%x)", deviceNumber, deviceIndex, info);
	return CELL_OK;
}

s32 cellVideoOutConvertCursorColor(u32 videoOut, s32 displaybuffer_format, f32 gamma, s32 source_buffer_format, vm::ptr<void> src_addr, vm::ptr<u32> dest_addr, s32 num)
{
	cellAvconfExt.todo("cellVideoOutConvertCursorColor(videoOut=%d, displaybuffer_format=0x%x, gamma=0x%x, source_buffer_format=0x%x, src_addr=*0x%x, dest_addr=*0x%x, num=0x%x)", videoOut, displaybuffer_format, gamma, source_buffer_format, src_addr, dest_addr, num);
	return CELL_OK;
}

s32 cellVideoOutGetGamma(u32 videoOut, vm::ptr<f32> gamma)
{
	cellAvconfExt.warning("cellVideoOutGetGamma(videoOut=%d, gamma=*0x%x)", videoOut, gamma);

	if (videoOut != CELL_VIDEO_OUT_PRIMARY)
	{
		return CELL_VIDEO_OUT_ERROR_UNSUPPORTED_VIDEO_OUT;
	}

	*gamma = *g_gamma;

	return CELL_OK;
}

s32 cellAudioInGetAvailableDeviceInfo(u32 count, vm::ptr<CellAudioInDeviceInfo> info)
{
	cellAvconfExt.todo("cellAudioInGetAvailableDeviceInfo(count=0x%x, info=*0x%x)", count, info);
	return 0; // number of available devices
}

s32 cellAudioOutGetAvailableDeviceInfo(u32 count, vm::ptr<CellAudioOutDeviceInfo2> info)
{
	cellAvconfExt.todo("cellAudioOutGetAvailableDeviceInfo(count=0x%x, info=*0x%x)", count, info);
	return 0; // number of available devices
}

s32 cellVideoOutSetGamma(u32 videoOut, f32 gamma)
{
	cellAvconfExt.warning("cellVideoOutSetGamma(videoOut=%d, gamma=%f)", videoOut, gamma);

	if (videoOut != CELL_VIDEO_OUT_PRIMARY)
	{
		return CELL_VIDEO_OUT_ERROR_UNSUPPORTED_VIDEO_OUT;
	}

	if (gamma < 0.8f || gamma > 1.2f)
	{
		return CELL_VIDEO_OUT_ERROR_ILLEGAL_PARAMETER;
	}

	*g_gamma = gamma;

	return CELL_OK;
}

s32 cellAudioOutRegisterDevice(u64 deviceType, vm::cptr<char> name, vm::ptr<CellAudioOutRegistrationOption> option, vm::ptr<CellAudioOutDeviceConfiguration> config)
{
	cellAvconfExt.todo("cellAudioOutRegisterDevice(deviceType=0x%llx, name=%s, option=*0x%x, config=*0x%x)", deviceType, name, option, config);
	return 0; // device number
}

s32 cellAudioOutSetDeviceMode(u32 deviceMode)
{
	cellAvconfExt.todo("cellAudioOutSetDeviceMode(deviceMode=0x%x)", deviceMode);
	return CELL_OK;
}

s32 cellAudioInSetDeviceMode(u32 deviceMode)
{
	cellAvconfExt.todo("cellAudioInSetDeviceMode(deviceMode=0x%x)", deviceMode);
	return CELL_OK;
}

s32 cellAudioInRegisterDevice(u64 deviceType, vm::cptr<char> name, vm::ptr<CellAudioInRegistrationOption> option, vm::ptr<CellAudioInDeviceConfiguration> config)
{
	cellAvconfExt.todo("cellAudioInRegisterDevice(deviceType=0x%llx, name=%s, option=*0x%x, config=*0x%x)", deviceType, name, option, config);
	return 0; // device number
}

s32 cellAudioInUnregisterDevice(u32 deviceNumber)
{
	cellAvconfExt.todo("cellAudioInUnregisterDevice(deviceNumber=0x%x)", deviceNumber);
	return CELL_OK;
}

s32 cellVideoOutGetScreenSize(u32 videoOut, vm::ptr<f32> screenSize)
{
	cellAvconfExt.warning("cellVideoOutGetScreenSize(videoOut=%d, screenSize=*0x%x)", videoOut, screenSize);

	if (videoOut != CELL_VIDEO_OUT_PRIMARY)
	{
		return CELL_VIDEO_OUT_ERROR_UNSUPPORTED_VIDEO_OUT;
	}

	//TODO: Use virtual screen size
#ifdef _WIN32
	//HDC screen = GetDC(NULL);
	//float diagonal = roundf(sqrtf((powf(float(GetDeviceCaps(screen, HORZSIZE)), 2) + powf(float(GetDeviceCaps(screen, VERTSIZE)), 2))) * 0.0393f);
#else
	// TODO: Linux implementation, without using wx
	// float diagonal = roundf(sqrtf((powf(wxGetDisplaySizeMM().GetWidth(), 2) + powf(wxGetDisplaySizeMM().GetHeight(), 2))) * 0.0393f);
#endif

	return CELL_VIDEO_OUT_ERROR_VALUE_IS_NOT_SET;
}

s32 cellVideoOutSetCopyControl(u32 videoOut, u32 control)
{
	cellAvconfExt.todo("cellVideoOutSetCopyControl(videoOut=%d, control=0x%x)", videoOut, control);
	return CELL_OK;
}


DECLARE(ppu_module_manager::cellAvconfExt)("cellSysutilAvconfExt", []()
{
	REG_VNID(cellSysutilAvconfExt, 0x00000000u, g_gamma).init = []
	{
		// Test
		*g_gamma = 1.0f;
	};

	REG_FUNC(cellSysutilAvconfExt, cellAudioOutUnregisterDevice);
	REG_FUNC(cellSysutilAvconfExt, cellAudioOutGetDeviceInfo2);
	REG_FUNC(cellSysutilAvconfExt, cellVideoOutSetXVColor);
	REG_FUNC(cellSysutilAvconfExt, cellVideoOutSetupDisplay);
	REG_FUNC(cellSysutilAvconfExt, cellAudioInGetDeviceInfo);
	REG_FUNC(cellSysutilAvconfExt, cellVideoOutConvertCursorColor);
	REG_FUNC(cellSysutilAvconfExt, cellVideoOutGetGamma);
	REG_FUNC(cellSysutilAvconfExt, cellAudioInGetAvailableDeviceInfo);
	REG_FUNC(cellSysutilAvconfExt, cellAudioOutGetAvailableDeviceInfo);
	REG_FUNC(cellSysutilAvconfExt, cellVideoOutSetGamma);
	REG_FUNC(cellSysutilAvconfExt, cellAudioOutRegisterDevice);
	REG_FUNC(cellSysutilAvconfExt, cellAudioOutSetDeviceMode);
	REG_FUNC(cellSysutilAvconfExt, cellAudioInSetDeviceMode);
	REG_FUNC(cellSysutilAvconfExt, cellAudioInRegisterDevice);
	REG_FUNC(cellSysutilAvconfExt, cellAudioInUnregisterDevice);
	REG_FUNC(cellSysutilAvconfExt, cellVideoOutGetScreenSize);
	REG_FUNC(cellSysutilAvconfExt, cellVideoOutSetCopyControl);
});
