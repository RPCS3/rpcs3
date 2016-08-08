#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"

#include "sceNp.h"
#include "sceNp2.h"

logs::channel sceNp2("sceNp2", logs::level::notice);

s32 sceNp2Init(u32 poolsize, vm::ptr<void> poolptr)
{
	sceNp2.warning("sceNp2Init(poolsize=0x%x, poolptr=*0x%x)", poolsize, poolptr);

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
	sceNp2.todo("sceNpMatching2Init(poolsize=0x%x, priority=%d)", poolsize, priority);

	return CELL_OK;
}

s32 sceNpMatching2Init2(u32 poolsize, s32 priority, vm::ptr<SceNpMatching2UtilityInitParam> param)
{
	sceNp2.todo("sceNpMatching2Init2(poolsize=0x%x, priority=%d, param=*0x%x)", poolsize, priority, param);

	// TODO:
	// 1. Create an internal thread
	// 2. Create heap area to be used by the NP matching 2 utility
	// 3. Set maximum lengths for the event data queues in the system

	return CELL_OK;
}

s32 sceNp2Term()
{
	sceNp2.warning("sceNp2Term()");

	return CELL_OK;
}

s32 sceNpMatching2Term(ppu_thread& ppu)
{
	sceNp2.warning("sceNpMatching2Term()");

	return CELL_OK;
}

s32 sceNpMatching2Term2()
{
	sceNp2.warning("sceNpMatching2Term2()");

	return CELL_OK;
}

s32 sceNpMatching2DestroyContext()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNpMatching2LeaveLobby()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNpMatching2RegisterLobbyMessageCallback()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNpMatching2GetWorldInfoList()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNpMatching2RegisterLobbyEventCallback()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNpMatching2GetLobbyMemberDataInternalList()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNpMatching2SearchRoom()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNpMatching2SignalingGetConnectionStatus()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNpMatching2SetUserInfo()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNpMatching2GetClanLobbyId()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNpMatching2GetLobbyMemberDataInternal()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNpMatching2ContextStart()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNpMatching2CreateServerContext()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNpMatching2GetMemoryInfo()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNpMatching2LeaveRoom()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNpMatching2SetRoomDataExternal()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNpMatching2SignalingGetConnectionInfo()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNpMatching2SendRoomMessage()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNpMatching2JoinLobby()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNpMatching2GetRoomMemberDataExternalList()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNpMatching2AbortRequest()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNpMatching2GetServerInfo()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNpMatching2GetEventData()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNpMatching2GetRoomSlotInfoLocal()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNpMatching2SendLobbyChatMessage()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNpMatching2AbortContextStart()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNpMatching2GetRoomMemberIdListLocal()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNpMatching2JoinRoom()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNpMatching2GetRoomMemberDataInternalLocal()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNpMatching2GetCbQueueInfo()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNpMatching2KickoutRoomMember()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNpMatching2ContextStartAsync()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNpMatching2SetSignalingOptParam()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNpMatching2RegisterContextCallback()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNpMatching2SendRoomChatMessage()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNpMatching2SetRoomDataInternal()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNpMatching2GetRoomDataInternal()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNpMatching2SignalingGetPingInfo()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNpMatching2GetServerIdListLocal()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNpUtilBuildCdnUrl()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNpMatching2GrantRoomOwner()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNpMatching2CreateContext()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNpMatching2GetSignalingOptParamLocal()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNpMatching2RegisterSignalingCallback()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNpMatching2ClearEventData()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNpMatching2GetUserInfoList()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNpMatching2GetRoomMemberDataInternal()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNpMatching2SetRoomMemberDataInternal()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNpMatching2JoinProhibitiveRoom()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNpMatching2SignalingSetCtxOpt()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNpMatching2DeleteServerContext()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNpMatching2SetDefaultRequestOptParam()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNpMatching2RegisterRoomEventCallback()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNpMatching2GetRoomPasswordLocal()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNpMatching2GetRoomDataExternalList()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNpMatching2CreateJoinRoom()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNpMatching2SignalingGetCtxOpt()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNpMatching2GetLobbyInfoList()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNpMatching2GetLobbyMemberIdListLocal()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNpMatching2SendLobbyInvitation()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNpMatching2ContextStop()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNpMatching2SetLobbyMemberDataInternal()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNpMatching2RegisterRoomMessageCallback()
{
	fmt::throw_exception("Unimplemented" HERE);
}


DECLARE(ppu_module_manager::sceNp2)("sceNp2", []()
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
