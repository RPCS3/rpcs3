#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"
#include "sceNp.h"

LOG_CHANNEL(sceNpMatchingInt);

error_code sceNpMatchingCancelRequest()
{
	UNIMPLEMENTED_FUNC(sceNpMatchingInt);
	return CELL_OK;
}

error_code sceNpMatchingGetRoomMemberList(u32 ctx_id, vm::ptr<SceNpRoomId> room_id, vm::ptr<u32> buflen, vm::ptr<void> buf)
{
	sceNpMatchingInt.warning("sceNpMatchingGetRoomMemberList(ctx_id=%d, room_id=*0x%x, buflen=*0x%x, buf=*0x%x)", ctx_id, room_id, buflen, buf);

	return matching_get_room_member_list(ctx_id, room_id, buflen, buf);
}

error_code sceNpMatchingJoinRoomWithoutGUI(u32 ctx_id, vm::ptr<SceNpRoomId> room_id, vm::ptr<SceNpMatchingGUIHandler> handler, vm::ptr<void> arg)
{
	sceNpMatchingInt.warning("sceNpMatchingJoinRoomWithoutGUI(ctx_id=%d, room_id=*0x%x, handler=*0x%x, arg=*0x%x)", ctx_id, room_id, handler, arg);

	return matching_join_room(ctx_id, room_id, handler, arg);
}

error_code OLD_sceNpMatchingJoinRoomGUI(u32 ctx_id, vm::ptr<SceNpRoomId> room_id, vm::ptr<SceNpMatchingGUIHandler> handler, vm::ptr<void> arg)
{
	sceNpMatchingInt.warning("OLD_sceNpMatchingJoinRoomGUI(ctx_id=%d, room_id=*0x%x, handler=*0x%x, arg=*0x%x)", ctx_id, room_id, handler, arg);

	return matching_join_room(ctx_id, room_id, handler, arg);
}

error_code OLD_sceNpMatchingSetRoomInfoNoLimit(u32 ctx_id, vm::ptr<SceNpLobbyId> lobby_id, vm::ptr<SceNpRoomId> room_id, vm::ptr<SceNpMatchingAttr> attr, vm::ptr<u32> req_id)
{
	sceNpMatchingInt.warning("OLD_sceNpMatchingSetRoomInfoNoLimit(ctx_id=%d, lobby_id=*0x%x, room_id=*0x%x, attr=*0x%x, req_id=*0x%x)", ctx_id, lobby_id, room_id, attr, req_id);

	return matching_set_room_info(ctx_id, lobby_id, room_id, attr, req_id, false);
}

error_code sceNpMatchingGetRoomListWithoutGUI(u32 ctx_id, vm::ptr<SceNpCommunicationId> communicationId, vm::ptr<SceNpMatchingReqRange> range, vm::ptr<SceNpMatchingSearchCondition> cond,
	vm::ptr<SceNpMatchingAttr> attr, vm::ptr<SceNpMatchingGUIHandler> handler, vm::ptr<void> arg)
{
	sceNpMatchingInt.warning("sceNpMatchingGetRoomListWithoutGUI(ctx_id=%d, communicationId=*0x%x, range=*0x%x, cond=*0x%x, attr=*0x%x, handler=*0x%x, arg=*0x%x)", ctx_id, communicationId, range, cond, attr, handler, arg);

	return matching_get_room_list(ctx_id, communicationId, range, cond, attr, handler, arg, false);
}

error_code sceNpMatchingGetRoomListGUI(u32 ctx_id, vm::ptr<SceNpCommunicationId> communicationId, vm::ptr<SceNpMatchingReqRange> range, vm::ptr<SceNpMatchingSearchCondition> cond,
	vm::ptr<SceNpMatchingAttr> attr, vm::ptr<SceNpMatchingGUIHandler> handler, vm::ptr<void> arg)
{
	sceNpMatchingInt.warning("sceNpMatchingGetRoomListGUI(ctx_id=%d, communicationId=*0x%x, range=*0x%x, cond=*0x%x, attr=*0x%x, handler=*0x%x, arg=*0x%x)", ctx_id, communicationId, range, cond, attr, handler, arg);

	return matching_get_room_list(ctx_id, communicationId, range, cond, attr, handler, arg, false);
}

error_code OLD_sceNpMatchingGetRoomInfoNoLimit(u32 ctx_id, vm::ptr<SceNpLobbyId> lobby_id, vm::ptr<SceNpRoomId> room_id, vm::ptr<SceNpMatchingAttr> attr, vm::ptr<u32> req_id)
{
	sceNpMatchingInt.warning("OLD_sceNpMatchingGetRoomInfoNoLimit(ctx_id=%d, lobby_id=*0x%x, room_id=*0x%x, attr=*0x%x, req_id=*0x%x)", ctx_id, lobby_id, room_id, attr, req_id);

	return matching_get_room_info(ctx_id, lobby_id, room_id, attr, req_id, false);
}

error_code sceNpMatchingCancelRequestGUI()
{
	UNIMPLEMENTED_FUNC(sceNpMatchingInt);
	return CELL_OK;
}

error_code sceNpMatchingSendRoomMessage()
{
	UNIMPLEMENTED_FUNC(sceNpMatchingInt);
	return CELL_OK;
}

error_code sceNpMatchingCreateRoomWithoutGUI(u32 ctx_id, vm::cptr<SceNpCommunicationId> communicationId, vm::cptr<SceNpMatchingAttr> attr, vm::ptr<SceNpMatchingGUIHandler> handler, vm::ptr<void> arg)
{
	sceNpMatchingInt.warning("sceNpMatchingCreateRoomWithoutGUI(ctx_id=%d, communicationId=*0x%x, attr=*0x%x, handler=*0x%x, arg=*0x%x)", ctx_id, communicationId, attr, handler, arg);

	return matching_create_room(ctx_id, communicationId, attr, handler, arg);
}

// This module has some conflicting function names with sceNp module, hence the REG_FNID
DECLARE(ppu_module_manager::sceNpMatchingInt)("sceNpMatchingInt", []()
{
	REG_FUNC(sceNpMatchingInt, sceNpMatchingCancelRequest);
	REG_FUNC(sceNpMatchingInt, sceNpMatchingGetRoomMemberList);
	REG_FUNC(sceNpMatchingInt, sceNpMatchingJoinRoomWithoutGUI);
	// REG_FUNC(sceNpMatchingInt, sceNpMatchingJoinRoomGUI);
	REG_FNID(sceNpMatchingInt, "sceNpMatchingJoinRoomGUI", OLD_sceNpMatchingJoinRoomGUI);
	// REG_FUNC(sceNpMatchingInt, sceNpMatchingSetRoomInfoNoLimit);
	REG_FNID(sceNpMatchingInt, "sceNpMatchingSetRoomInfoNoLimit", OLD_sceNpMatchingSetRoomInfoNoLimit);
	REG_FUNC(sceNpMatchingInt, sceNpMatchingGetRoomListWithoutGUI);
	REG_FUNC(sceNpMatchingInt, sceNpMatchingGetRoomListGUI);
	// REG_FUNC(sceNpMatchingInt, sceNpMatchingGetRoomInfoNoLimit);
	REG_FNID(sceNpMatchingInt, "sceNpMatchingGetRoomInfoNoLimit", OLD_sceNpMatchingGetRoomInfoNoLimit);
	REG_FUNC(sceNpMatchingInt, sceNpMatchingCancelRequestGUI);
	REG_FUNC(sceNpMatchingInt, sceNpMatchingSendRoomMessage);
	REG_FUNC(sceNpMatchingInt, sceNpMatchingCreateRoomWithoutGUI);
});
