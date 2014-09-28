#include "stdafx.h"
#include "Ini.h"
#include "rpcs3.h"
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

	if (!videoOut == CELL_VIDEO_OUT_PRIMARY)
		return CELL_VIDEO_OUT_ERROR_UNSUPPORTED_VIDEO_OUT;

	// Calculate screen's diagonal size in inches
	u32 diagonal = round(sqrt((pow(wxGetDisplaySizeMM().GetWidth(), 2) + pow(wxGetDisplaySizeMM().GetHeight(), 2))) * 0.0393);

	if (Ini.GS3DTV.GetValue())
	{
		*screenSize = diagonal;
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