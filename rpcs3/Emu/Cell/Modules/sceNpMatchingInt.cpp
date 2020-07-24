#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"

LOG_CHANNEL(sceNpMatchingInt);

error_code sceNpMatchingCancelRequest()
{
	UNIMPLEMENTED_FUNC(sceNpMatchingInt);
	return CELL_OK;
}

error_code sceNpMatchingGetRoomMemberList()
{
	UNIMPLEMENTED_FUNC(sceNpMatchingInt);
	return CELL_OK;
}

error_code sceNpMatchingJoinRoomWithoutGUI()
{
	UNIMPLEMENTED_FUNC(sceNpMatchingInt);
	return CELL_OK;
}

// Parameter "unknown" added to distinguish this function
// from the one in sceNp.cpp which has the same name
error_code sceNpMatchingJoinRoomGUI(vm::ptr<void> unknown)
{
	UNIMPLEMENTED_FUNC(sceNpMatchingInt);
	return CELL_OK;
}

error_code sceNpMatchingSetRoomInfoNoLimit(vm::ptr<void> unknown)
{
	UNIMPLEMENTED_FUNC(sceNpMatchingInt);
	return CELL_OK;
}

error_code sceNpMatchingGetRoomListWithoutGUI()
{
	UNIMPLEMENTED_FUNC(sceNpMatchingInt);
	return CELL_OK;
}

error_code sceNpMatchingGetRoomListGUI()
{
	UNIMPLEMENTED_FUNC(sceNpMatchingInt);
	return CELL_OK;
}

error_code sceNpMatchingGetRoomInfoNoLimit(vm::ptr<void> unknown)
{
	UNIMPLEMENTED_FUNC(sceNpMatchingInt);
	return CELL_OK;
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

error_code sceNpMatchingCreateRoomWithoutGUI()
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
