#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/ARMv7/PSVFuncList.h"

extern psv_log_base sceNpMatching;

#define REG_FUNC(nid, name) reg_psv_func(nid, &sceNpMatching, #name, name)

psv_log_base sceNpMatching("SceNpMatching2", []()
{
	sceNpMatching.on_load = nullptr;
	sceNpMatching.on_unload = nullptr;
	sceNpMatching.on_stop = nullptr;

	//REG_FUNC(0xEBB1FE74, sceNpMatching2Init);
	//REG_FUNC(0x0124641C, sceNpMatching2Term);
	//REG_FUNC(0xADF578E1, sceNpMatching2CreateContext);
	//REG_FUNC(0x368AA759, sceNpMatching2DestroyContext);
	//REG_FUNC(0xBB2E7559, sceNpMatching2ContextStart);
	//REG_FUNC(0xF2847E3B, sceNpMatching2AbortContextStart);
	//REG_FUNC(0x506454DE, sceNpMatching2ContextStop);
	//REG_FUNC(0xF3A43C50, sceNpMatching2SetDefaultRequestOptParam);
	//REG_FUNC(0xF486991B, sceNpMatching2RegisterRoomEventCallback);
	//REG_FUNC(0xFA51949B, sceNpMatching2RegisterRoomMessageCallback);
	//REG_FUNC(0xF9E35566, sceNpMatching2RegisterContextCallback);
	//REG_FUNC(0x74EB6CE9, sceNpMatching2AbortRequest);
	//REG_FUNC(0x7BD39E50, sceNpMatching2GetMemoryInfo);
	//REG_FUNC(0x65C0FEED, sceNpMatching2GetServerLocal);
	//REG_FUNC(0xC086B560, sceNpMatching2GetWorldInfoList);
	//REG_FUNC(0x818A9499, sceNpMatching2CreateJoinRoom);
	//REG_FUNC(0xD48BAF13, sceNpMatching2SearchRoom);
	//REG_FUNC(0x33F7D5AE, sceNpMatching2JoinRoom);
	//REG_FUNC(0xC8B0C9EE, sceNpMatching2LeaveRoom);
	//REG_FUNC(0x495D2B46, sceNpMatching2GetSignalingOptParamLocal);
	//REG_FUNC(0xE0BE0510, sceNpMatching2SendRoomChatMessage);
	//REG_FUNC(0x7B908D99, sceNpMatching2SendRoomMessage);
	//REG_FUNC(0x4E4C55BD, sceNpMatching2SignalingGetConnectionStatus);
	//REG_FUNC(0x20598618, sceNpMatching2SignalingGetConnectionInfo);
	//REG_FUNC(0x79310806, sceNpMatching2SignalingGetLocalNetInfo);
	//REG_FUNC(0xF0CB1DD3, sceNpMatching2SignalingGetPeerNetInfo);
	//REG_FUNC(0xADCD102C, sceNpMatching2SignalingCancelPeerNetInfo);
	//REG_FUNC(0xFDC7B2C9, sceNpMatching2SignalingGetPeerNetInfoResult);
	//REG_FUNC(0x1C60BC5B, sceNpMatching2RegisterSignalingCallback);
	//REG_FUNC(0x8F88AC7E, sceNpMatching2SetRoomDataExternal);
	//REG_FUNC(0xA8021394, sceNpMatching2KickoutRoomMember);
});
