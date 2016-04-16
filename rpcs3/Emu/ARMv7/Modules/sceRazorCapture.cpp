#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/ARMv7/ARMv7Module.h"

#include "sceRazorCapture.h"

LOG_CHANNEL(sceRazorCapture);

void sceRazorCaptureSetTrigger(u32 frameIndex, vm::cptr<char> captureFilename)
{
	throw EXCEPTION("");
}

void sceRazorCaptureSetTriggerNextFrame(vm::cptr<char> captureFilename)
{
	throw EXCEPTION("");
}

b8 sceRazorCaptureIsInProgress()
{
	throw EXCEPTION("");
}

#define REG_FUNC(nid, name) REG_FNID(SceRazorCapture, nid, name)

DECLARE(arm_module_manager::SceRazorCapture)("SceRazorCapture", []()
{
	REG_FUNC(0x911E0AA0, sceRazorCaptureIsInProgress);
	REG_FUNC(0xE916B538, sceRazorCaptureSetTrigger);
	REG_FUNC(0x3D4B7E68, sceRazorCaptureSetTriggerNextFrame);
});
