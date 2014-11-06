#include "stdafx.h"
#include "Ini.h"
#include "Emu/Memory/Memory.h"
#include "Emu/SysCalls/Modules.h"
#include "Emu/RSX/sysutil_video.h"

Module *cellAvconfExt = nullptr;

int cellVideoOutConvertCursorColor()
{
	UNIMPLEMENTED_FUNC(cellAvconfExt);
	return CELL_OK;
}

int cellVideoOutGetScreenSize(u32 videoOut, vm::ptr<float> screenSize)
{
	cellAvconfExt->Warning("cellVideoOutGetScreenSize(videoOut=%d, screenSize_addr=0x%x)", videoOut, screenSize.addr());

	if (videoOut != CELL_VIDEO_OUT_PRIMARY)
		return CELL_VIDEO_OUT_ERROR_UNSUPPORTED_VIDEO_OUT;

#ifdef _WIN32
	HDC screen = GetDC(NULL);
	float diagonal = roundf(sqrtf((powf(GetDeviceCaps(screen, HORZSIZE), 2) + powf(GetDeviceCaps(screen, VERTSIZE), 2))) * 0.0393);
#else
	// TODO: Linux implementation, without using wx
	// float diagonal = roundf(sqrtf((powf(wxGetDisplaySizeMM().GetWidth(), 2) + powf(wxGetDisplaySizeMM().GetHeight(), 2))) * 0.0393);
#endif

	if (Ini.GS3DTV.GetValue())
	{
#ifdef _WIN32
		*screenSize = diagonal;
#endif
		return CELL_OK;
	}

	return CELL_VIDEO_OUT_ERROR_VALUE_IS_NOT_SET;
}

int cellVideoOutGetGamma()
{
	UNIMPLEMENTED_FUNC(cellAvconfExt);
	return CELL_OK;
}

int cellVideoOutSetGamma()
{
	UNIMPLEMENTED_FUNC(cellAvconfExt);
	return CELL_OK;
}

void cellAvconfExt_init(Module *pxThis)
{
	cellAvconfExt = pxThis;

	cellAvconfExt->AddFunc(0x4ec8c141, cellVideoOutConvertCursorColor);
	cellAvconfExt->AddFunc(0xfaa275a4, cellVideoOutGetScreenSize);
	cellAvconfExt->AddFunc(0xc7020f62, cellVideoOutSetGamma);
}