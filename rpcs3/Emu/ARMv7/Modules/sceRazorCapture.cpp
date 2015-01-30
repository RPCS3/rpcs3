#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/ARMv7/PSVFuncList.h"

extern psv_log_base sceRazorCapture;

void sceRazorCaptureSetTrigger(u32 frameIndex, vm::psv::ptr<const char> captureFilename)
{
	throw __FUNCTION__;
}

void sceRazorCaptureSetTriggerNextFrame(vm::psv::ptr<const char> captureFilename)
{
	throw __FUNCTION__;
}

bool sceRazorCaptureIsInProgress()
{
	throw __FUNCTION__;
}

#define REG_FUNC(nid, name) reg_psv_func(nid, &sceRazorCapture, #name, name)

psv_log_base sceRazorCapture("SceRazorCapture", []()
{
	sceRazorCapture.on_load = nullptr;
	sceRazorCapture.on_unload = nullptr;
	sceRazorCapture.on_stop = nullptr;

	REG_FUNC(0x911E0AA0, sceRazorCaptureIsInProgress);
	REG_FUNC(0xE916B538, sceRazorCaptureSetTrigger);
	REG_FUNC(0x3D4B7E68, sceRazorCaptureSetTriggerNextFrame);
});
