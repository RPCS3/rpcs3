#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/PSP2/ARMv7Module.h"

#include "sceVoice.h"

logs::channel sceVoice("sceVoice");

s32 sceVoiceInit(vm::ptr<SceVoiceInitParam> pArg, SceVoiceVersion version)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceVoiceEnd()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceVoiceStart(vm::ptr<SceVoiceStartParam> pArg)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceVoiceStop()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceVoiceResetPort(u32 portId)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceVoiceCreatePort(vm::ptr<u32> portId, vm::cptr<SceVoicePortParam> pArg)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceVoiceUpdatePort(u32 portId, vm::cptr<SceVoicePortParam> pArg)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceVoiceConnectIPortToOPort(u32 ips, u32 ops)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceVoiceDisconnectIPortFromOPort(u32 ips, u32 ops)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceVoiceDeletePort(u32 portId)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceVoiceWriteToIPort(u32 ips, vm::cptr<void> data, vm::ptr<u32> size, s16 frameGaps)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceVoiceReadFromOPort(u32 ops, vm::ptr<void> data, vm::ptr<u32> size)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceVoiceSetMuteFlagAll(u16 bMuted)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceVoiceSetMuteFlag(u32 portId, u16 bMuted)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceVoiceGetMuteFlag(u32 portId, vm::ptr<u16> bMuted)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceVoiceSetVolume(u32 portId, float volume)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceVoiceGetVolume(u32 portId, vm::ptr<float> volume)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceVoiceSetBitRate(u32 portId, SceVoiceBitRate bitrate)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceVoiceGetBitRate(u32 portId, vm::ptr<u32> bitrate)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceVoiceGetPortInfo(u32 portId, vm::ptr<SceVoiceBasePortInfo> pInfo)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceVoicePausePort(u32 portId)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceVoiceResumePort(u32 portId)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceVoicePausePortAll()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceVoiceResumePortAll()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceVoiceGetResourceInfo(vm::ptr<SceVoiceResourceInfo> pInfo)
{
	fmt::throw_exception("Unimplemented" HERE);
}


#define REG_FUNC(nid, name) REG_FNID(SceVoice, nid, name)

DECLARE(arm_module_manager::SceVoice)("SceVoice", []()
{
	REG_FUNC(0xD02C00B4, sceVoiceGetBitRate);
	REG_FUNC(0xC913F7E9, sceVoiceGetMuteFlag);
	REG_FUNC(0x875CC80D, sceVoiceGetVolume);
	REG_FUNC(0x02F58D6F, sceVoiceSetBitRate);
	REG_FUNC(0x0B9E4AE2, sceVoiceSetMuteFlag);
	REG_FUNC(0xDB90EAC4, sceVoiceSetMuteFlagAll);
	//REG_FUNC(0xD93769E6, sceVoiceSetVolume);
	REG_FUNC(0x6E46950E, sceVoiceGetResourceInfo);
	REG_FUNC(0xAC98853E, sceVoiceEnd);
	REG_FUNC(0x805CC20F, sceVoiceInit);
	REG_FUNC(0xB2ED725B, sceVoiceStart);
	REG_FUNC(0xC3868DF6, sceVoiceStop);
	REG_FUNC(0x698BDAAE, sceVoiceConnectIPortToOPort);
	REG_FUNC(0xFA4E57B1, sceVoiceCreatePort);
	REG_FUNC(0xAE46564D, sceVoiceDeletePort);
	REG_FUNC(0x5F0260F4, sceVoiceDisconnectIPortFromOPort);
	REG_FUNC(0x5933CCFB, sceVoiceGetPortInfo);
	REG_FUNC(0x23C6B16B, sceVoicePausePort);
	REG_FUNC(0x39AA3884, sceVoicePausePortAll);
	REG_FUNC(0x09E4D18C, sceVoiceReadFromOPort);
	REG_FUNC(0x5E1CE910, sceVoiceResetPort);
	REG_FUNC(0x2DE35411, sceVoiceResumePort);
	REG_FUNC(0x1F93FC0C, sceVoiceResumePortAll);
	REG_FUNC(0xCE855C50, sceVoiceUpdatePort);
	REG_FUNC(0x0A22EC0E, sceVoiceWriteToIPort);
});
