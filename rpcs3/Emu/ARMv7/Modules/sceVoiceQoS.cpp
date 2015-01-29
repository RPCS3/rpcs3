#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/ARMv7/PSVFuncList.h"

extern psv_log_base sceVoiceQoS;

#define REG_FUNC(nid, name) reg_psv_func(nid, &sceVoiceQoS, #name, name)

psv_log_base sceVoiceQoS("SceVoiceQos", []()
{
	sceVoiceQoS.on_load = nullptr;
	sceVoiceQoS.on_unload = nullptr;
	sceVoiceQoS.on_stop = nullptr;

	//REG_FUNC(0x4B5FFF1C, sceVoiceQoSInit);
	//REG_FUNC(0xFB0B747B, sceVoiceQoSEnd);
	//REG_FUNC(0xAAB54BE4, sceVoiceQoSCreateLocalEndpoint);
	//REG_FUNC(0x68FABF6F, sceVoiceQoSDeleteLocalEndpoint);
	//REG_FUNC(0xBAB98727, sceVoiceQoSCreateRemoteEndpoint);
	//REG_FUNC(0xC2F2C771, sceVoiceQoSDeleteRemoteEndpoint);
	//REG_FUNC(0xE0C5CEEE, sceVoiceQoSConnect);
	//REG_FUNC(0x3C7A08B0, sceVoiceQoSDisconnect);
	//REG_FUNC(0xE5B4527D, sceVoiceQoSGetLocalEndpoint);
	//REG_FUNC(0x876A9B9C, sceVoiceQoSGetRemoteEndpoint);
	//REG_FUNC(0x540CEBA5, sceVoiceQoSSetLocalEndpointAttribute);
	//REG_FUNC(0xC981AB3B, sceVoiceQoSGetLocalEndpointAttribute);
	//REG_FUNC(0xE757806F, sceVoiceQoSSetConnectionAttribute);
	//REG_FUNC(0xE81B8D44, sceVoiceQoSGetConnectionAttribute);
	//REG_FUNC(0xC9DC1425, sceVoiceQoSGetStatus);
	//REG_FUNC(0x2FE1F28F, sceVoiceQoSWritePacket);
	//REG_FUNC(0x2D613549, sceVoiceQoSReadPacket);
});
