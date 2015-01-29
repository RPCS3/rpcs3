#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/ARMv7/PSVFuncList.h"

extern psv_log_base sceVoice;

#define REG_FUNC(nid, name) reg_psv_func(nid, &sceVoice, #name, name)

psv_log_base sceVoice("SceVoice", []()
{
	sceVoice.on_load = nullptr;
	sceVoice.on_unload = nullptr;
	sceVoice.on_stop = nullptr;

	//REG_FUNC(0xD02C00B4, sceVoiceGetBitRate);
	//REG_FUNC(0xC913F7E9, sceVoiceGetMuteFlag);
	//REG_FUNC(0x875CC80D, sceVoiceGetVolume);
	//REG_FUNC(0x02F58D6F, sceVoiceSetBitRate);
	//REG_FUNC(0x0B9E4AE2, sceVoiceSetMuteFlag);
	//REG_FUNC(0xDB90EAC4, sceVoiceSetMuteFlagAll);
	//REG_FUNC(0xD93769E6, sceVoiceSetVolume);
	//REG_FUNC(0x6E46950E, sceVoiceGetResourceInfo);
	//REG_FUNC(0xAC98853E, sceVoiceEnd);
	//REG_FUNC(0x805CC20F, sceVoiceInit);
	//REG_FUNC(0xB2ED725B, sceVoiceStart);
	//REG_FUNC(0xC3868DF6, sceVoiceStop);
	//REG_FUNC(0x698BDAAE, sceVoiceConnectIPortToOPort);
	//REG_FUNC(0xFA4E57B1, sceVoiceCreatePort);
	//REG_FUNC(0xAE46564D, sceVoiceDeletePort);
	//REG_FUNC(0x5F0260F4, sceVoiceDisconnectIPortFromOPort);
	//REG_FUNC(0x5933CCFB, sceVoiceGetPortInfo);
	//REG_FUNC(0x23C6B16B, sceVoicePausePort);
	//REG_FUNC(0x39AA3884, sceVoicePausePortAll);
	//REG_FUNC(0x09E4D18C, sceVoiceReadFromOPort);
	//REG_FUNC(0x5E1CE910, sceVoiceResetPort);
	//REG_FUNC(0x2DE35411, sceVoiceResumePort);
	//REG_FUNC(0x1F93FC0C, sceVoiceResumePortAll);
	//REG_FUNC(0xCE855C50, sceVoiceUpdatePort);
	//REG_FUNC(0x0A22EC0E, sceVoiceWriteToIPort);
});
