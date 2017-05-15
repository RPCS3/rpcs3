#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/PSP2/ARMv7Module.h"

#include "sceVoiceQoS.h"

logs::channel sceVoiceQoS("sceVoiceQoS");

s32 sceVoiceQoSInit()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceVoiceQoSEnd()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceVoiceQoSCreateLocalEndpoint(vm::ptr<s32> pLocalId)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceVoiceQoSDeleteLocalEndpoint(s32 localId)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceVoiceQoSCreateRemoteEndpoint(vm::ptr<s32> pRemoteId)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceVoiceQoSDeleteRemoteEndpoint(s32 remoteId)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceVoiceQoSConnect(vm::ptr<s32> pConnectionId, s32 localId, s32 remoteId)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceVoiceQoSDisconnect(s32 connectionId)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceVoiceQoSGetLocalEndpoint(s32 connectionId, vm::ptr<s32> pLocalId)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceVoiceQoSGetRemoteEndpoint(s32 connectionId, vm::ptr<s32> pRemoteId)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceVoiceQoSSetLocalEndpointAttribute(s32 localId, SceVoiceQoSAttributeId attributeId, vm::cptr<void> pAttributeValue, s32 attributeSize)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceVoiceQoSGetLocalEndpointAttribute(s32 localId, SceVoiceQoSAttributeId attributeId, vm::ptr<void> pAttributeValue, s32 attributeSize)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceVoiceQoSSetConnectionAttribute(s32 connectionId, SceVoiceQoSAttributeId attributeId, vm::cptr<void> pAttributeValue, s32 attributeSize)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceVoiceQoSGetConnectionAttribute(s32 connectionId, SceVoiceQoSAttributeId attributeId, vm::ptr<void> pAttributeValue, s32 attributeSize)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceVoiceQoSGetStatus(s32 connectionId, SceVoiceQoSStatusId statusId, vm::ptr<void> pStatusValue, s32 statusSize)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceVoiceQoSWritePacket(s32 connectionId, vm::cptr<void> pData, vm::ptr<u32> pSize)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceVoiceQoSReadPacket(s32 connectionId, vm::ptr<void> pData, vm::ptr<u32> pSize)
{
	fmt::throw_exception("Unimplemented" HERE);
}


#define REG_FUNC(nid, name) REG_FNID(SceVoiceQoS, nid, name)

DECLARE(arm_module_manager::SceVoiceQoS)("SceVoiceQoS", []()
{
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
