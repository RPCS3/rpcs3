#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/Cell/PPUModule.h"

#include "cellAudioIn.h"
#include "cellAudioOut.h"
#include "cellVideoOut.h"

logs::channel cellAvconfExt("cellAvconfExt", logs::level::notice);

vm::gvar<f32> g_gamma; // TODO

s32 cellAudioOutUnregisterDevice()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellAudioOutGetDeviceInfo2()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellVideoOutSetXVColor()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellVideoOutSetupDisplay()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellAudioInGetDeviceInfo()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellVideoOutConvertCursorColor()
{
	fmt::throw_exception("Unimplemented" HERE);
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

s32 cellAudioInGetAvailableDeviceInfo()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellAudioOutGetAvailableDeviceInfo()
{
	fmt::throw_exception("Unimplemented" HERE);
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

s32 cellAudioOutRegisterDevice()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellAudioOutSetDeviceMode()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellAudioInSetDeviceMode()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellAudioInRegisterDevice()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellAudioInUnregisterDevice()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellVideoOutGetScreenSize(u32 videoOut, vm::ptr<float> screenSize)
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


DECLARE(ppu_module_manager::cellAvconfExt)("cellSysutilAvconfExt", []()
{
	REG_VNID(cellSysutilAvconfExt, 0x00000000, g_gamma).init = []
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
});
