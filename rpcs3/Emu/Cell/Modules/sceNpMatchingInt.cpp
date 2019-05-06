#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"

LOG_CHANNEL(sceNpMatchingInt);

s32 sceNpMatchingCancelRequest()
{
	UNIMPLEMENTED_FUNC(sceNpMatchingInt);
	return CELL_OK;
}

s32 sceNpMatchingGetRoomMemberList()
{
	UNIMPLEMENTED_FUNC(sceNpMatchingInt);
	return CELL_OK;
}

s32 sceNpMatchingJoinRoomWithoutGUI()
{
	UNIMPLEMENTED_FUNC(sceNpMatchingInt);
	return CELL_OK;
}

// Parameter "unknown" added to distinguish this function
// from the one in sceNp.cpp which has the same name
s32 sceNpMatchingJoinRoomGUI(vm::ptr<void> unknown)
{
	UNIMPLEMENTED_FUNC(sceNpMatchingInt);
	return CELL_OK;
}

s32 sceNpMatchingSetRoomInfoNoLimit(vm::ptr<void> unknown)
{
	UNIMPLEMENTED_FUNC(sceNpMatchingInt);
	return CELL_OK;
}

s32 sceNpMatchingGetRoomListWithoutGUI()
{
	UNIMPLEMENTED_FUNC(sceNpMatchingInt);
	return CELL_OK;
}

s32 sceNpMatchingGetRoomListGUI()
{
	UNIMPLEMENTED_FUNC(sceNpMatchingInt);
	return CELL_OK;
}

s32 sceNpMatchingGetRoomInfoNoLimit(vm::ptr<void> unknown)
{
	UNIMPLEMENTED_FUNC(sceNpMatchingInt);
	return CELL_OK;
}

s32 sceNpMatchingCancelRequestGUI()
{
	UNIMPLEMENTED_FUNC(sceNpMatchingInt);
	return CELL_OK;
}

s32 sceNpMatchingSendRoomMessage()
{
	UNIMPLEMENTED_FUNC(sceNpMatchingInt);
	return CELL_OK;
}

s32 sceNpMatchingCreateRoomWithoutGUI()
{
	UNIMPLEMENTED_FUNC(sceNpMatchingInt);
	return CELL_OK;
}

DECLARE(ppu_module_manager::sceNpMatchingInt)("sceNpMatchingInt", []()
{
	REG_FUNC(sceNpMatchingInt, sceNpMatchingCancelRequest);
	REG_FUNC(sceNpMatchingInt, sceNpMatchingGetRoomMemberList);
	REG_FUNC(sceNpMatchingInt, sceNpMatchingJoinRoomWithoutGUI);
	REG_FUNC(sceNpMatchingInt, sceNpMatchingJoinRoomGUI);
	REG_FUNC(sceNpMatchingInt, sceNpMatchingSetRoomInfoNoLimit);
	REG_FUNC(sceNpMatchingInt, sceNpMatchingGetRoomListWithoutGUI);
	REG_FUNC(sceNpMatchingInt, sceNpMatchingGetRoomListGUI);
	REG_FUNC(sceNpMatchingInt, sceNpMatchingGetRoomInfoNoLimit);
	REG_FUNC(sceNpMatchingInt, sceNpMatchingCancelRequestGUI);
	REG_FUNC(sceNpMatchingInt, sceNpMatchingSendRoomMessage);
	REG_FUNC(sceNpMatchingInt, sceNpMatchingCreateRoomWithoutGUI);
});
