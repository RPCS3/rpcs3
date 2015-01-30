#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/ARMv7/PSVFuncList.h"

extern psv_log_base sceVoice;

enum SceVoicePortType : s32
{
	SCEVOICE_PORTTYPE_NULL = -1,
	SCEVOICE_PORTTYPE_IN_DEVICE = 0,
	SCEVOICE_PORTTYPE_IN_PCMAUDIO = 1,
	SCEVOICE_PORTTYPE_IN_VOICE = 2,
	SCEVOICE_PORTTYPE_OUT_PCMAUDIO = 3,
	SCEVOICE_PORTTYPE_OUT_VOICE = 4,
	SCEVOICE_PORTTYPE_OUT_DEVICE = 5
};

enum SceVoicePortState : s32
{
	SCEVOICE_PORTSTATE_NULL = -1,
	SCEVOICE_PORTSTATE_IDLE = 0,
	SCEVOICE_PORTSTATE_READY = 1,
	SCEVOICE_PORTSTATE_BUFFERING = 2,
	SCEVOICE_PORTSTATE_RUNNING = 3
};

enum SceVoiceBitRate : s32
{
	SCEVOICE_BITRATE_NULL = -1,
	SCEVOICE_BITRATE_3850 = 3850,
	SCEVOICE_BITRATE_4650 = 4650,
	SCEVOICE_BITRATE_5700 = 5700,
	SCEVOICE_BITRATE_7300 = 7300
};

enum SceVoiceSamplingRate : s32
{
	SCEVOICE_SAMPLINGRATE_NULL = -1,
	SCEVOICE_SAMPLINGRATE_16000 = 16000
};

enum SceVoicePcmDataType : s32
{
	SCEVOICE_PCM_NULL = -1,
	SCEVOICE_PCM_SHORT_LITTLE_ENDIAN = 0
};

enum SceVoiceVersion : s32
{
	SCEVOICE_VERSION_100 = 100
};

enum SceVoiceAppType : s32
{
	SCEVOICE_APPTYPE_GAME = 1 << 29
};

struct SceVoicePCMFormat
{
	SceVoicePcmDataType dataType;
	SceVoiceSamplingRate sampleRate;
};

struct SceVoiceResourceInfo
{
	u16 maxInVoicePort;
	u16 maxOutVoicePort;
	u16 maxInDevicePort;
	u16 maxOutDevicePort;
	u16 maxTotalPort;
};

struct SceVoiceBasePortInfo
{
	SceVoicePortType portType;
	SceVoicePortState state;
	vm::psv::ptr<u32> pEdge;
	u32 numByte;
	u32 frameSize;
	u16 numEdge;
	u16 reserved;
};

struct SceVoicePortParam
{
	SceVoicePortType portType;
	u16 threshold;
	u16 bMute;
	float volume;

	union
	{
		struct
		{
			SceVoiceBitRate bitrate;
		} voice;

		struct
		{
			u32 bufSize;
			SceVoicePCMFormat format;
		} pcmaudio;


		struct
		{
			u32 playerId;
		} device;
	};
};

typedef vm::psv::ptr<void(vm::psv::ptr<void> event)> SceVoiceEventCallback;

struct SceVoiceInitParam
{
	s32 appType;
	SceVoiceEventCallback onEvent;
	u8 reserved[24];
};

struct SceVoiceStartParam
{
	s32 container;
	u8 reserved[28];
};

s32 sceVoiceInit(vm::psv::ptr<SceVoiceInitParam> pArg, SceVoiceVersion version)
{
	throw __FUNCTION__;
}

s32 sceVoiceEnd()
{
	throw __FUNCTION__;
}

s32 sceVoiceStart(vm::psv::ptr<SceVoiceStartParam> pArg)
{
	throw __FUNCTION__;
}

s32 sceVoiceStop()
{
	throw __FUNCTION__;
}

s32 sceVoiceResetPort(u32 portId)
{
	throw __FUNCTION__;
}

s32 sceVoiceCreatePort(vm::psv::ptr<u32> portId, vm::psv::ptr<const SceVoicePortParam> pArg)
{
	throw __FUNCTION__;
}

s32 sceVoiceUpdatePort(u32 portId, vm::psv::ptr<const SceVoicePortParam> pArg)
{
	throw __FUNCTION__;
}

s32 sceVoiceConnectIPortToOPort(u32 ips, u32 ops)
{
	throw __FUNCTION__;
}

s32 sceVoiceDisconnectIPortFromOPort(u32 ips, u32 ops)
{
	throw __FUNCTION__;
}

s32 sceVoiceDeletePort(u32 portId)
{
	throw __FUNCTION__;
}

s32 sceVoiceWriteToIPort(u32 ips, vm::psv::ptr<const void> data, vm::psv::ptr<u32> size, int16_t frameGaps)
{
	throw __FUNCTION__;
}

s32 sceVoiceReadFromOPort(u32 ops, vm::psv::ptr<void> data, vm::psv::ptr<u32> size)
{
	throw __FUNCTION__;
}

s32 sceVoiceSetMuteFlagAll(u16 bMuted)
{
	throw __FUNCTION__;
}

s32 sceVoiceSetMuteFlag(u32 portId, u16 bMuted)
{
	throw __FUNCTION__;
}

s32 sceVoiceGetMuteFlag(u32 portId, vm::psv::ptr<u16> bMuted)
{
	throw __FUNCTION__;
}

//s32 sceVoiceSetVolume(u32 portId, float volume)
//{
//	throw __FUNCTION__;
//}

s32 sceVoiceGetVolume(u32 portId, vm::psv::ptr<float> volume)
{
	throw __FUNCTION__;
}

s32 sceVoiceSetBitRate(u32 portId, SceVoiceBitRate bitrate)
{
	throw __FUNCTION__;
}

s32 sceVoiceGetBitRate(u32 portId, vm::psv::ptr<u32> bitrate)
{
	throw __FUNCTION__;
}

s32 sceVoiceGetPortInfo(u32 portId, vm::psv::ptr<SceVoiceBasePortInfo> pInfo)
{
	throw __FUNCTION__;
}

s32 sceVoicePausePort(u32 portId)
{
	throw __FUNCTION__;
}

s32 sceVoiceResumePort(u32 portId)
{
	throw __FUNCTION__;
}

s32 sceVoicePausePortAll()
{
	throw __FUNCTION__;
}

s32 sceVoiceResumePortAll()
{
	throw __FUNCTION__;
}

s32 sceVoiceGetResourceInfo(vm::psv::ptr<SceVoiceResourceInfo> pInfo)
{
	throw __FUNCTION__;
}


#define REG_FUNC(nid, name) reg_psv_func(nid, &sceVoice, #name, name)

psv_log_base sceVoice("SceVoice", []()
{
	sceVoice.on_load = nullptr;
	sceVoice.on_unload = nullptr;
	sceVoice.on_stop = nullptr;

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
