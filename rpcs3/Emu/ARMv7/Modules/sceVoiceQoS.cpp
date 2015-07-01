#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/ARMv7/PSVFuncList.h"

#include "sceVoiceQoS.h"

s32 sceVoiceQoSInit()
{
	throw EXCEPTION("");
}

s32 sceVoiceQoSEnd()
{
	throw EXCEPTION("");
}

s32 sceVoiceQoSCreateLocalEndpoint(vm::ptr<s32> pLocalId)
{
	throw EXCEPTION("");
}

s32 sceVoiceQoSDeleteLocalEndpoint(s32 localId)
{
	throw EXCEPTION("");
}

s32 sceVoiceQoSCreateRemoteEndpoint(vm::ptr<s32> pRemoteId)
{
	throw EXCEPTION("");
}

s32 sceVoiceQoSDeleteRemoteEndpoint(s32 remoteId)
{
	throw EXCEPTION("");
}

s32 sceVoiceQoSConnect(vm::ptr<s32> pConnectionId, s32 localId, s32 remoteId)
{
	throw EXCEPTION("");
}

s32 sceVoiceQoSDisconnect(s32 connectionId)
{
	throw EXCEPTION("");
}

s32 sceVoiceQoSGetLocalEndpoint(s32 connectionId, vm::ptr<s32> pLocalId)
{
	throw EXCEPTION("");
}

s32 sceVoiceQoSGetRemoteEndpoint(s32 connectionId, vm::ptr<s32> pRemoteId)
{
	throw EXCEPTION("");
}

s32 sceVoiceQoSSetLocalEndpointAttribute(s32 localId, SceVoiceQoSAttributeId attributeId, vm::cptr<void> pAttributeValue, s32 attributeSize)
{
	throw EXCEPTION("");
}

s32 sceVoiceQoSGetLocalEndpointAttribute(s32 localId, SceVoiceQoSAttributeId attributeId, vm::ptr<void> pAttributeValue, s32 attributeSize)
{
	throw EXCEPTION("");
}

s32 sceVoiceQoSSetConnectionAttribute(s32 connectionId, SceVoiceQoSAttributeId attributeId, vm::cptr<void> pAttributeValue, s32 attributeSize)
{
	throw EXCEPTION("");
}

s32 sceVoiceQoSGetConnectionAttribute(s32 connectionId, SceVoiceQoSAttributeId attributeId, vm::ptr<void> pAttributeValue, s32 attributeSize)
{
	throw EXCEPTION("");
}

s32 sceVoiceQoSGetStatus(s32 connectionId, SceVoiceQoSStatusId statusId, vm::ptr<void> pStatusValue, s32 statusSize)
{
	throw EXCEPTION("");
}

s32 sceVoiceQoSWritePacket(s32 connectionId, vm::cptr<void> pData, vm::ptr<u32> pSize)
{
	throw EXCEPTION("");
}

s32 sceVoiceQoSReadPacket(s32 connectionId, vm::ptr<void> pData, vm::ptr<u32> pSize)
{
	throw EXCEPTION("");
}


#define REG_FUNC(nid, name) reg_psv_func(nid, &sceVoiceQoS, #name, name)

psv_log_base sceVoiceQoS("SceVoiceQos", []()
{
	sceVoiceQoS.on_load = nullptr;
	sceVoiceQoS.on_unload = nullptr;
	sceVoiceQoS.on_stop = nullptr;
	sceVoiceQoS.on_error = nullptr;

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
