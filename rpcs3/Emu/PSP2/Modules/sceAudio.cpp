#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/PSP2/ARMv7Module.h"

#include "sceAudio.h"

logs::channel sceAudio("sceAudio");

s32 sceAudioOutOpenPort(s32 portType, s32 len, s32 freq, s32 param)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceAudioOutReleasePort(s32 port)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceAudioOutOutput(s32 port, vm::ptr<void> ptr)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceAudioOutSetVolume(s32 port, s32 flag, vm::ptr<s32> vol)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceAudioOutSetConfig(s32 port, s32 len, s32 freq, s32 param)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceAudioOutGetConfig(s32 port, s32 configType)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceAudioOutGetRestSample(s32 port)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceAudioOutGetAdopt(s32 portType)
{
	fmt::throw_exception("Unimplemented" HERE);
}


#define REG_FUNC(nid, name) REG_FNID(SceAudio, nid, name)

DECLARE(arm_module_manager::SceAudio)("SceAudio", []()
{
	REG_FUNC(0x5BC341E4, sceAudioOutOpenPort);
	REG_FUNC(0x69E2E6B5, sceAudioOutReleasePort);
	REG_FUNC(0x02DB3F5F, sceAudioOutOutput);
	REG_FUNC(0x64167F11, sceAudioOutSetVolume);
	REG_FUNC(0xB8BA0D07, sceAudioOutSetConfig);
	REG_FUNC(0x9C8EDAEA, sceAudioOutGetConfig);
	REG_FUNC(0x9A5370C4, sceAudioOutGetRestSample);
	REG_FUNC(0x12FB1767, sceAudioOutGetAdopt);
	//REG_FUNC(0xC6D8D775, sceAudioInRaw);
});
