#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/ARMv7/PSVFuncList.h"

#include "sceRazorCapture.h"

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

#define REG_FUNC(nid, name) reg_psv_func(nid, &sceRazorCapture, #name, name)

psv_log_base sceRazorCapture("SceRazorCapture", []()
{
	sceRazorCapture.on_load = nullptr;
	sceRazorCapture.on_unload = nullptr;
	sceRazorCapture.on_stop = nullptr;
	sceRazorCapture.on_error = nullptr;

	REG_FUNC(0x911E0AA0, sceRazorCaptureIsInProgress);
	REG_FUNC(0xE916B538, sceRazorCaptureSetTrigger);
	REG_FUNC(0x3D4B7E68, sceRazorCaptureSetTriggerNextFrame);
});
