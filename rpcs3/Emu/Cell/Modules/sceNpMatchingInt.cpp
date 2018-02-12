#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/Cell/PPUModule.h"



logs::channel sceNpMatchingInt("sceNpMatchingInt");

s32 sceNpMatchingGetRoomMemberList()
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

s32 sceNpMatchingGetRoomListGUI()
{
	UNIMPLEMENTED_FUNC(sceNpMatchingInt);
	return CELL_OK;
}

s32 sceNpMatchingSendRoomMessage()
{
	UNIMPLEMENTED_FUNC(sceNpMatchingInt);
	return CELL_OK;
}

DECLARE(ppu_module_manager::sceNpMatchingInt)("sceNpMatchingInt", []()
{
	REG_FUNC(sceNpMatchingInt, sceNpMatchingGetRoomMemberList);
	REG_FUNC(sceNpMatchingInt, sceNpMatchingJoinRoomGUI);
	REG_FUNC(sceNpMatchingInt, sceNpMatchingGetRoomListGUI);
	REG_FUNC(sceNpMatchingInt, sceNpMatchingSendRoomMessage);
});
