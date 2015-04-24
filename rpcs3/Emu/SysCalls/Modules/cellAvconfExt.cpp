#include "stdafx.h"
#include "Ini.h"
#include "Emu/Memory/Memory.h"
#include "Emu/SysCalls/Modules.h"
#include "Emu/RSX/sysutil_video.h"

extern Module cellAvconfExt;

s32 cellVideoOutConvertCursorColor()
{
	UNIMPLEMENTED_FUNC(cellAvconfExt);
	return CELL_OK;
}

s32 cellVideoOutGetScreenSize(u32 videoOut, vm::ptr<float> screenSize)
{
	cellAvconfExt.Warning("cellVideoOutGetScreenSize(videoOut=%d, screenSize=*0x%x)", videoOut, screenSize);

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

s32 cellVideoOutGetGamma()
{
	UNIMPLEMENTED_FUNC(cellAvconfExt);
	return CELL_OK;
}

s32 cellVideoOutSetGamma()
{
	UNIMPLEMENTED_FUNC(cellAvconfExt);
	return CELL_OK;
}

Module cellAvconfExt("cellAvconfExt", []()
{
	REG_FUNC(cellAvconfExt, cellVideoOutConvertCursorColor);
	REG_FUNC(cellAvconfExt, cellVideoOutGetScreenSize);
	REG_FUNC(cellAvconfExt, cellVideoOutGetGamma);
	REG_FUNC(cellAvconfExt, cellVideoOutSetGamma);
});
