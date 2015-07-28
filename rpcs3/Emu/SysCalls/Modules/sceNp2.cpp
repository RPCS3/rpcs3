#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/SysCalls/Modules.h"

#include "sceNp.h"
#include "sceNp2.h"

extern Module sceNp2;

s32 sceNp2Init(u32 poolsize, vm::ptr<void> poolptr)
{
	sceNp2.Warning("sceNp2Init(poolsize=0x%x, poolptr=*0x%x)", poolsize, poolptr);

	if (poolsize == 0)
	{
		return SCE_NP_ERROR_INVALID_ARGUMENT;
	}
	else if (poolsize < 128 * 1024)
	{
		return SCE_NP_ERROR_INSUFFICIENT_BUFFER;
	}

	if (!poolptr)
	{
		return SCE_NP_ERROR_INVALID_ARGUMENT;
	}

	return CELL_OK;
}

s32 sceNpMatching2Init(u32 poolsize, s32 priority)
{
	sceNp2.Todo("sceNpMatching2Init(poolsize=0x%x, priority=%d)", poolsize, priority);

	return CELL_OK;
}

s32 sceNpMatching2Init2(u32 poolsize, s32 priority, vm::ptr<SceNpMatching2UtilityInitParam> param)
{
	sceNp2.Todo("sceNpMatching2Init2(poolsize=0x%x, priority=%d, param=*0x%x)", poolsize, priority, param);

	// TODO:
	// 1. Create an internal thread
	// 2. Create heap area to be used by the NP matching 2 utility
	// 3. Set maximum lengths for the event data queues in the system

	return CELL_OK;
}

s32 sceNp2Term()
{
	sceNp2.Warning("sceNp2Term()");

	return CELL_OK;
}

s32 sceNpMatching2Term(PPUThread& ppu)
{
	sceNp2.Warning("sceNpMatching2Term()");

	return CELL_OK;
}

s32 sceNpMatching2Term2()
{
	sceNp2.Warning("sceNpMatching2Term2()");

	return CELL_OK;
}

s32 sceNpMatching2DestroyContext()
{
	throw EXCEPTION("");
}

s32 sceNpMatching2LeaveLobby()
{
	throw EXCEPTION("");
}

s32 sceNpMatching2RegisterLobbyMessageCallback()
{
	throw EXCEPTION("");
}

s32 sceNpMatching2GetWorldInfoList()
{
	throw EXCEPTION("");
}

s32 sceNpMatching2RegisterLobbyEventCallback()
{
	throw EXCEPTION("");
}

s32 sceNpMatching2GetLobbyMemberDataInternalList()
{
	throw EXCEPTION("");
}

s32 sceNpMatching2SearchRoom()
{
	throw EXCEPTION("");
}

s32 sceNpMatching2SignalingGetConnectionStatus()
{
	throw EXCEPTION("");
}

s32 sceNpMatching2SetUserInfo()
{
	throw EXCEPTION("");
}

s32 sceNpMatching2GetClanLobbyId()
{
	throw EXCEPTION("");
}

s32 sceNpMatching2GetLobbyMemberDataInternal()
{
	throw EXCEPTION("");
}

s32 sceNpMatching2ContextStart()
{
	throw EXCEPTION("");
}

s32 sceNpMatching2CreateServerContext()
{
	throw EXCEPTION("");
}

s32 sceNpMatching2GetMemoryInfo()
{
	throw EXCEPTION("");
}

s32 sceNpMatching2LeaveRoom()
{
	throw EXCEPTION("");
}

s32 sceNpMatching2SetRoomDataExternal()
{
	throw EXCEPTION("");
}

s32 sceNpMatching2SignalingGetConnectionInfo()
{
	throw EXCEPTION("");
}

s32 sceNpMatching2SendRoomMessage()
{
	throw EXCEPTION("");
}

s32 sceNpMatching2JoinLobby()
{
	throw EXCEPTION("");
}

s32 sceNpMatching2GetRoomMemberDataExternalList()
{
	throw EXCEPTION("");
}

s32 sceNpMatching2AbortRequest()
{
	throw EXCEPTION("");
}

s32 sceNpMatching2GetServerInfo()
{
	throw EXCEPTION("");
}

s32 sceNpMatching2GetEventData()
{
	throw EXCEPTION("");
}

s32 sceNpMatching2GetRoomSlotInfoLocal()
{
	throw EXCEPTION("");
}

s32 sceNpMatching2SendLobbyChatMessage()
{
	throw EXCEPTION("");
}

s32 sceNpMatching2AbortContextStart()
{
	throw EXCEPTION("");
}

s32 sceNpMatching2GetRoomMemberIdListLocal()
{
	throw EXCEPTION("");
}

s32 sceNpMatching2JoinRoom()
{
	throw EXCEPTION("");
}

s32 sceNpMatching2GetRoomMemberDataInternalLocal()
{
	throw EXCEPTION("");
}

s32 sceNpMatching2GetCbQueueInfo()
{
	throw EXCEPTION("");
}

s32 sceNpMatching2KickoutRoomMember()
{
	throw EXCEPTION("");
}

s32 sceNpMatching2ContextStartAsync()
{
	throw EXCEPTION("");
}

s32 sceNpMatching2SetSignalingOptParam()
{
	throw EXCEPTION("");
}

s32 sceNpMatching2RegisterContextCallback()
{
	throw EXCEPTION("");
}

s32 sceNpMatching2SendRoomChatMessage()
{
	throw EXCEPTION("");
}

s32 sceNpMatching2SetRoomDataInternal()
{
	throw EXCEPTION("");
}

s32 sceNpMatching2GetRoomDataInternal()
{
	throw EXCEPTION("");
}

s32 sceNpMatching2SignalingGetPingInfo()
{
	throw EXCEPTION("");
}

s32 sceNpMatching2GetServerIdListLocal()
{
	throw EXCEPTION("");
}

s32 sceNpUtilBuildCdnUrl()
{
	throw EXCEPTION("");
}

s32 sceNpMatching2GrantRoomOwner()
{
	throw EXCEPTION("");
}

s32 sceNpMatching2CreateContext()
{
	throw EXCEPTION("");
}

s32 sceNpMatching2GetSignalingOptParamLocal()
{
	throw EXCEPTION("");
}

s32 sceNpMatching2RegisterSignalingCallback()
{
	throw EXCEPTION("");
}

s32 sceNpMatching2ClearEventData()
{
	throw EXCEPTION("");
}

s32 sceNpMatching2GetUserInfoList()
{
	throw EXCEPTION("");
}

s32 sceNpMatching2GetRoomMemberDataInternal()
{
	throw EXCEPTION("");
}

s32 sceNpMatching2SetRoomMemberDataInternal()
{
	throw EXCEPTION("");
}

s32 sceNpMatching2JoinProhibitiveRoom()
{
	throw EXCEPTION("");
}

s32 sceNpMatching2SignalingSetCtxOpt()
{
	throw EXCEPTION("");
}

s32 sceNpMatching2DeleteServerContext()
{
	throw EXCEPTION("");
}

s32 sceNpMatching2SetDefaultRequestOptParam()
{
	throw EXCEPTION("");
}

s32 sceNpMatching2RegisterRoomEventCallback()
{
	throw EXCEPTION("");
}

s32 sceNpMatching2GetRoomPasswordLocal()
{
	throw EXCEPTION("");
}

s32 sceNpMatching2GetRoomDataExternalList()
{
	throw EXCEPTION("");
}

s32 sceNpMatching2CreateJoinRoom()
{
	throw EXCEPTION("");
}

s32 sceNpMatching2SignalingGetCtxOpt()
{
	throw EXCEPTION("");
}

s32 sceNpMatching2GetLobbyInfoList()
{
	throw EXCEPTION("");
}

s32 sceNpMatching2GetLobbyMemberIdListLocal()
{
	throw EXCEPTION("");
}

s32 sceNpMatching2SendLobbyInvitation()
{
	throw EXCEPTION("");
}

s32 sceNpMatching2ContextStop()
{
	throw EXCEPTION("");
}

s32 sceNpMatching2SetLobbyMemberDataInternal()
{
	throw EXCEPTION("");
}

s32 sceNpMatching2RegisterRoomMessageCallback()
{
	throw EXCEPTION("");
}


Module sceNp2("sceNp2", []()
{
	REG_FUNC(sceNp2, sceNpMatching2DestroyContext);
	REG_FUNC(sceNp2, sceNpMatching2LeaveLobby);
	REG_FUNC(sceNp2, sceNpMatching2RegisterLobbyMessageCallback);
	REG_FUNC(sceNp2, sceNpMatching2GetWorldInfoList);
	REG_FUNC(sceNp2, sceNpMatching2RegisterLobbyEventCallback);
	REG_FUNC(sceNp2, sceNpMatching2GetLobbyMemberDataInternalList);
	REG_FUNC(sceNp2, sceNpMatching2SearchRoom);
	REG_FUNC(sceNp2, sceNpMatching2SignalingGetConnectionStatus);
	REG_FUNC(sceNp2, sceNpMatching2SetUserInfo);
	REG_FUNC(sceNp2, sceNpMatching2GetClanLobbyId);
	REG_FUNC(sceNp2, sceNpMatching2GetLobbyMemberDataInternal);
	REG_FUNC(sceNp2, sceNpMatching2ContextStart);
	REG_FUNC(sceNp2, sceNpMatching2CreateServerContext);
	REG_FUNC(sceNp2, sceNpMatching2GetMemoryInfo);
	REG_FUNC(sceNp2, sceNpMatching2LeaveRoom);
	REG_FUNC(sceNp2, sceNpMatching2SetRoomDataExternal);
	REG_FUNC(sceNp2, sceNpMatching2Term2);
	REG_FUNC(sceNp2, sceNpMatching2SignalingGetConnectionInfo);
	REG_FUNC(sceNp2, sceNpMatching2SendRoomMessage);
	REG_FUNC(sceNp2, sceNpMatching2JoinLobby);
	REG_FUNC(sceNp2, sceNpMatching2GetRoomMemberDataExternalList);
	REG_FUNC(sceNp2, sceNpMatching2AbortRequest);
	REG_FUNC(sceNp2, sceNpMatching2Term);
	REG_FUNC(sceNp2, sceNpMatching2GetServerInfo);
	REG_FUNC(sceNp2, sceNpMatching2GetEventData);
	REG_FUNC(sceNp2, sceNpMatching2GetRoomSlotInfoLocal);
	REG_FUNC(sceNp2, sceNpMatching2SendLobbyChatMessage);
	REG_FUNC(sceNp2, sceNpMatching2Init);
	REG_FUNC(sceNp2, sceNp2Init);
	REG_FUNC(sceNp2, sceNpMatching2AbortContextStart);
	REG_FUNC(sceNp2, sceNpMatching2GetRoomMemberIdListLocal);
	REG_FUNC(sceNp2, sceNpMatching2JoinRoom);
	REG_FUNC(sceNp2, sceNpMatching2GetRoomMemberDataInternalLocal);
	REG_FUNC(sceNp2, sceNpMatching2GetCbQueueInfo);
	REG_FUNC(sceNp2, sceNpMatching2KickoutRoomMember);
	REG_FUNC(sceNp2, sceNpMatching2ContextStartAsync);
	REG_FUNC(sceNp2, sceNpMatching2SetSignalingOptParam);
	REG_FUNC(sceNp2, sceNpMatching2RegisterContextCallback);
	REG_FUNC(sceNp2, sceNpMatching2SendRoomChatMessage);
	REG_FUNC(sceNp2, sceNpMatching2SetRoomDataInternal);
	REG_FUNC(sceNp2, sceNpMatching2GetRoomDataInternal);
	REG_FUNC(sceNp2, sceNpMatching2SignalingGetPingInfo);
	REG_FUNC(sceNp2, sceNpMatching2GetServerIdListLocal);
	REG_FUNC(sceNp2, sceNpUtilBuildCdnUrl);
	REG_FUNC(sceNp2, sceNpMatching2GrantRoomOwner);
	REG_FUNC(sceNp2, sceNpMatching2CreateContext);
	REG_FUNC(sceNp2, sceNpMatching2GetSignalingOptParamLocal);
	REG_FUNC(sceNp2, sceNpMatching2RegisterSignalingCallback);
	REG_FUNC(sceNp2, sceNpMatching2ClearEventData);
	REG_FUNC(sceNp2, sceNp2Term);
	REG_FUNC(sceNp2, sceNpMatching2GetUserInfoList);
	REG_FUNC(sceNp2, sceNpMatching2GetRoomMemberDataInternal);
	REG_FUNC(sceNp2, sceNpMatching2SetRoomMemberDataInternal);
	REG_FUNC(sceNp2, sceNpMatching2JoinProhibitiveRoom);
	REG_FUNC(sceNp2, sceNpMatching2SignalingSetCtxOpt);
	REG_FUNC(sceNp2, sceNpMatching2DeleteServerContext);
	REG_FUNC(sceNp2, sceNpMatching2SetDefaultRequestOptParam);
	REG_FUNC(sceNp2, sceNpMatching2RegisterRoomEventCallback);
	REG_FUNC(sceNp2, sceNpMatching2GetRoomPasswordLocal);
	REG_FUNC(sceNp2, sceNpMatching2GetRoomDataExternalList);
	REG_FUNC(sceNp2, sceNpMatching2CreateJoinRoom);
	REG_FUNC(sceNp2, sceNpMatching2SignalingGetCtxOpt);
	REG_FUNC(sceNp2, sceNpMatching2GetLobbyInfoList);
	REG_FUNC(sceNp2, sceNpMatching2GetLobbyMemberIdListLocal);
	REG_FUNC(sceNp2, sceNpMatching2SendLobbyInvitation);
	REG_FUNC(sceNp2, sceNpMatching2ContextStop);
	REG_FUNC(sceNp2, sceNpMatching2Init2);
	REG_FUNC(sceNp2, sceNpMatching2SetLobbyMemberDataInternal);
	REG_FUNC(sceNp2, sceNpMatching2RegisterRoomMessageCallback);
});
