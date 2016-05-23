#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/PSP2/ARMv7Module.h"

#include "sceAudioIn.h"

logs::channel sceAudioIn("sceAudioIn", logs::level::notice);

s32 sceAudioInOpenPort(s32 portType, s32 grain, s32 freq, s32 param)
{
	throw EXCEPTION("");
}

s32 sceAudioInReleasePort(s32 port)
{
	throw EXCEPTION("");
}

s32 sceAudioInInput(s32 port, vm::ptr<void> destPtr)
{
	throw EXCEPTION("");
}


#define REG_FUNC(nid, name) REG_FNID(SceAudioIn, nid, name)

DECLARE(arm_module_manager::SceAudioIn)("SceAudioIn", []()
{
	REG_FUNC(0x638ADD2D, sceAudioInInput);
	REG_FUNC(0x39B50DC1, sceAudioInOpenPort);
	REG_FUNC(0x3A61B8C4, sceAudioInReleasePort);
	//REG_FUNC(0x566AC433, sceAudioInGetAdopt);
});
