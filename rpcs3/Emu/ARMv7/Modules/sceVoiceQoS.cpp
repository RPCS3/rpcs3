#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/ARMv7/PSVFuncList.h"

extern psv_log_base sceVoiceQoS;

typedef s32 SceVoiceQoSLocalId;
typedef s32 SceVoiceQoSRemoteId;
typedef s32 SceVoiceQoSConnectionId;

enum SceVoiceQoSAttributeId : s32
{
	SCE_VOICE_QOS_ATTR_MIC_VOLUME,
	SCE_VOICE_QOS_ATTR_MIC_MUTE,
	SCE_VOICE_QOS_ATTR_SPEAKER_VOLUME,
	SCE_VOICE_QOS_ATTR_SPEAKER_MUTE,
	SCE_VOICE_QOS_ATTR_DESIRED_OUT_BIT_RATE
};

enum SceVoiceQoSStatusId : s32
{
	SCE_VOICE_QOS_IN_BITRATE,
	SCE_VOICE_QOS_OUT_BITRATE,
	SCE_VOICE_QOS_OUT_READ_BITRATE,
	SCE_VOICE_QOS_IN_FRAME_RECEIVED_RATIO,
	SCE_VOICE_QOS_HEARTBEAT_FLAG
};

s32 sceVoiceQoSInit()
{
	throw __FUNCTION__;
}

s32 sceVoiceQoSEnd()
{
	throw __FUNCTION__;
}

s32 sceVoiceQoSCreateLocalEndpoint(vm::psv::ptr<SceVoiceQoSLocalId> pLocalId)
{
	throw __FUNCTION__;
}

s32 sceVoiceQoSDeleteLocalEndpoint(SceVoiceQoSLocalId localId)
{
	throw __FUNCTION__;
}

s32 sceVoiceQoSCreateRemoteEndpoint(vm::psv::ptr<SceVoiceQoSRemoteId> pRemoteId)
{
	throw __FUNCTION__;
}

s32 sceVoiceQoSDeleteRemoteEndpoint(SceVoiceQoSRemoteId remoteId)
{
	throw __FUNCTION__;
}

s32 sceVoiceQoSConnect(vm::psv::ptr<SceVoiceQoSConnectionId> pConnectionId, SceVoiceQoSLocalId localId, SceVoiceQoSRemoteId remoteId)
{
	throw __FUNCTION__;
}

s32 sceVoiceQoSDisconnect(SceVoiceQoSConnectionId connectionId)
{
	throw __FUNCTION__;
}

s32 sceVoiceQoSGetLocalEndpoint(SceVoiceQoSConnectionId connectionId, vm::psv::ptr<SceVoiceQoSLocalId> pLocalId)
{
	throw __FUNCTION__;
}

s32 sceVoiceQoSGetRemoteEndpoint(SceVoiceQoSConnectionId connectionId, vm::psv::ptr<SceVoiceQoSRemoteId> pRemoteId)
{
	throw __FUNCTION__;
}

s32 sceVoiceQoSSetLocalEndpointAttribute(SceVoiceQoSLocalId localId, SceVoiceQoSAttributeId attributeId, vm::psv::ptr<const void> pAttributeValue, s32 attributeSize)
{
	throw __FUNCTION__;
}

s32 sceVoiceQoSGetLocalEndpointAttribute(SceVoiceQoSLocalId localId, SceVoiceQoSAttributeId attributeId, vm::psv::ptr<void> pAttributeValue, s32 attributeSize)
{
	throw __FUNCTION__;
}

s32 sceVoiceQoSSetConnectionAttribute(SceVoiceQoSConnectionId connectionId, SceVoiceQoSAttributeId attributeId, vm::psv::ptr<const void> pAttributeValue, s32 attributeSize)
{
	throw __FUNCTION__;
}

s32 sceVoiceQoSGetConnectionAttribute(SceVoiceQoSConnectionId connectionId, SceVoiceQoSAttributeId attributeId, vm::psv::ptr<void> pAttributeValue, s32 attributeSize)
{
	throw __FUNCTION__;
}

s32 sceVoiceQoSGetStatus(SceVoiceQoSConnectionId connectionId, SceVoiceQoSStatusId statusId, vm::psv::ptr<void> pStatusValue, s32 statusSize)
{
	throw __FUNCTION__;
}

s32 sceVoiceQoSWritePacket(SceVoiceQoSConnectionId connectionId, vm::psv::ptr<const void> pData, vm::psv::ptr<u32> pSize)
{
	throw __FUNCTION__;
}

s32 sceVoiceQoSReadPacket(SceVoiceQoSConnectionId connectionId, vm::psv::ptr<void> pData, vm::psv::ptr<u32> pSize)
{
	throw __FUNCTION__;
}


#define REG_FUNC(nid, name) reg_psv_func<name>(nid, &sceVoiceQoS, #name, name)

psv_log_base sceVoiceQoS("SceVoiceQos", []()
{
	sceVoiceQoS.on_load = nullptr;
	sceVoiceQoS.on_unload = nullptr;
	sceVoiceQoS.on_stop = nullptr;

	REG_FUNC(0x4B5FFF1C, sceVoiceQoSInit);
	REG_FUNC(0xFB0B747B, sceVoiceQoSEnd);
	REG_FUNC(0xAAB54BE4, sceVoiceQoSCreateLocalEndpoint);
	REG_FUNC(0x68FABF6F, sceVoiceQoSDeleteLocalEndpoint);
	REG_FUNC(0xBAB98727, sceVoiceQoSCreateRemoteEndpoint);
	REG_FUNC(0xC2F2C771, sceVoiceQoSDeleteRemoteEndpoint);
	REG_FUNC(0xE0C5CEEE, sceVoiceQoSConnect);
	REG_FUNC(0x3C7A08B0, sceVoiceQoSDisconnect);
	REG_FUNC(0xE5B4527D, sceVoiceQoSGetLocalEndpoint);
	REG_FUNC(0x876A9B9C, sceVoiceQoSGetRemoteEndpoint);
	REG_FUNC(0x540CEBA5, sceVoiceQoSSetLocalEndpointAttribute);
	REG_FUNC(0xC981AB3B, sceVoiceQoSGetLocalEndpointAttribute);
	REG_FUNC(0xE757806F, sceVoiceQoSSetConnectionAttribute);
	REG_FUNC(0xE81B8D44, sceVoiceQoSGetConnectionAttribute);
	REG_FUNC(0xC9DC1425, sceVoiceQoSGetStatus);
	REG_FUNC(0x2FE1F28F, sceVoiceQoSWritePacket);
	REG_FUNC(0x2D613549, sceVoiceQoSReadPacket);
});
