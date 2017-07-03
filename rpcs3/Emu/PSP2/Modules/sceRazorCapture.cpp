#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/PSP2/ARMv7Module.h"

#include "sceRazorCapture.h"

logs::channel sceRazorCapture("sceRazorCapture");

void sceRazorCaptureSetTrigger(u32 frameIndex, vm::cptr<char> captureFilename)
{
	fmt::throw_exception("Unimplemented" HERE);
}

void sceRazorCaptureSetTriggerNextFrame(vm::cptr<char> captureFilename)
{
	fmt::throw_exception("Unimplemented" HERE);
}

b8 sceRazorCaptureIsInProgress()
{
	fmt::throw_exception("Unimplemented" HERE);
}

#define REG_FUNC(nid, name) REG_FNID(SceRazorCapture, nid, name)

DECLARE(arm_module_manager::SceRazorCapture)("SceRazorCapture", []()
{
	REG_FUNC(0x911E0AA0, sceRazorCaptureIsInProgress);
	REG_FUNC(0xE916B538, sceRazorCaptureSetTrigger);
	REG_FUNC(0x3D4B7E68, sceRazorCaptureSetTriggerNextFrame);
});
