#include "stdafx.h"
#include "Emu/IdManager.h"
#include "Emu/Memory/Memory.h"
#include "Emu/SysCalls/Modules.h"

#include "Ini.h"
#include "cellAvconfExt.h"
#include "cellAudioIn.h"
#include "cellAudioOut.h"
#include "cellVideoOut.h"

extern Module cellAvconfExt;

s32 cellAudioOutUnregisterDevice()
{
	throw EXCEPTION("");
}

s32 cellAudioOutGetDeviceInfo2()
{
	throw EXCEPTION("");
}

s32 cellVideoOutSetXVColor()
{
	throw EXCEPTION("");
}

s32 cellVideoOutSetupDisplay()
{
	throw EXCEPTION("");
}

s32 cellAudioInGetDeviceInfo()
{
	throw EXCEPTION("");
}

s32 cellVideoOutConvertCursorColor()
{
	throw EXCEPTION("");
}

s32 cellVideoOutGetGamma(u32 videoOut, vm::ptr<u8> gamma)
{
	cellAvconfExt.Warning("cellVideoOutGetGamma(videoOut=%d, gamma=*0x%x)", videoOut, gamma);

	if (videoOut != CELL_VIDEO_OUT_PRIMARY)
	{
		return CELL_VIDEO_OUT_ERROR_UNSUPPORTED_VIDEO_OUT;
	}

	const auto avconfExt = fxm::get<avconfext_t>();

	*gamma = avconfExt->gamma;

	return CELL_VIDEO_OUT_SUCCEEDED;
}

s32 cellAudioInGetAvailableDeviceInfo()
{
	throw EXCEPTION("");
}

s32 cellAudioOutGetAvailableDeviceInfo()
{
	throw EXCEPTION("");
}

s32 cellVideoOutSetGamma(u32 videoOut, u8 gamma)
{
	cellAvconfExt.Warning("cellVideoOutSetGamma(videoOut=%d, gamma=%d)", videoOut, gamma);

	if (videoOut != CELL_VIDEO_OUT_PRIMARY)
	{
		return CELL_VIDEO_OUT_ERROR_UNSUPPORTED_VIDEO_OUT;
	}

	if (gamma < 0.8 || gamma > 1.2)
	{
		return CELL_VIDEO_OUT_ERROR_ILLEGAL_PARAMETER;
	}

	const auto avconfExt = fxm::get<avconfext_t>();

	avconfExt->gamma = gamma;

	return CELL_VIDEO_OUT_SUCCEEDED;
}

s32 cellAudioOutRegisterDevice()
{
	throw EXCEPTION("");
}

s32 cellAudioOutSetDeviceMode()
{
	throw EXCEPTION("");
}

s32 cellAudioInSetDeviceMode()
{
	throw EXCEPTION("");
}

s32 cellAudioInRegisterDevice()
{
	throw EXCEPTION("");
}

s32 cellAudioInUnregisterDevice()
{
	throw EXCEPTION("");
}

s32 cellVideoOutGetScreenSize(u32 videoOut, vm::ptr<float> screenSize)
{
	cellAvconfExt.Todo("cellVideoOutGetScreenSize(videoOut=%d, screenSize=*0x%x)", videoOut, screenSize);

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

	if (Ini.GS3DTV.GetValue())
	{
		*screenSize = 24.0f;
		return CELL_OK;
	}

	return CELL_VIDEO_OUT_ERROR_VALUE_IS_NOT_SET;
}


Module cellAvconfExt("cellAvconfExt", []()
{
	fxm::remove<avconfext_t>();
	fxm::make<avconfext_t>();

	REG_FUNC(cellAvconfExt, cellAudioOutUnregisterDevice);
	REG_FUNC(cellAvconfExt, cellAudioOutGetDeviceInfo2);
	REG_FUNC(cellAvconfExt, cellVideoOutSetXVColor);
	REG_FUNC(cellAvconfExt, cellVideoOutSetupDisplay);
	REG_FUNC(cellAvconfExt, cellAudioInGetDeviceInfo);
	REG_FUNC(cellAvconfExt, cellVideoOutConvertCursorColor);
	REG_FUNC(cellAvconfExt, cellVideoOutGetGamma);
	REG_FUNC(cellAvconfExt, cellAudioInGetAvailableDeviceInfo);
	REG_FUNC(cellAvconfExt, cellAudioOutGetAvailableDeviceInfo);
	REG_FUNC(cellAvconfExt, cellVideoOutSetGamma);
	REG_FUNC(cellAvconfExt, cellAudioOutRegisterDevice);
	REG_FUNC(cellAvconfExt, cellAudioOutSetDeviceMode);
	REG_FUNC(cellAvconfExt, cellAudioInSetDeviceMode);
	REG_FUNC(cellAvconfExt, cellAudioInRegisterDevice);
	REG_FUNC(cellAvconfExt, cellAudioInUnregisterDevice);
	REG_FUNC(cellAvconfExt, cellVideoOutGetScreenSize);
});
