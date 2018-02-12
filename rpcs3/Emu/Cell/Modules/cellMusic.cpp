#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/IdManager.h"
#include "Emu/Cell/PPUModule.h"

#include "cellMusic.h"
#include "cellSysutil.h"



logs::channel cellMusic("cellMusic");

struct music2_t
{
	vm::ptr<CellMusic2Callback> func;
	vm::ptr<void> userData;
};

s32 cellMusicGetSelectionContext()
{
	UNIMPLEMENTED_FUNC(cellMusic);
	return CELL_OK;
}

s32 cellMusicSetSelectionContext2()
{
	UNIMPLEMENTED_FUNC(cellMusic);
	return CELL_OK;
}

s32 cellMusicSetVolume2()
{
	UNIMPLEMENTED_FUNC(cellMusic);
	return CELL_OK;
}

s32 cellMusicGetContentsId()
{
	UNIMPLEMENTED_FUNC(cellMusic);
	return CELL_OK;
}

s32 cellMusicSetSelectionContext()
{
	UNIMPLEMENTED_FUNC(cellMusic);
	return CELL_OK;
}

s32 cellMusicInitialize2SystemWorkload()
{
	UNIMPLEMENTED_FUNC(cellMusic);
	return CELL_OK;
}

s32 cellMusicGetPlaybackStatus2()
{
	UNIMPLEMENTED_FUNC(cellMusic);
	return CELL_OK;
}

s32 cellMusicGetContentsId2()
{
	UNIMPLEMENTED_FUNC(cellMusic);
	return CELL_OK;
}

s32 cellMusicFinalize()
{
	UNIMPLEMENTED_FUNC(cellMusic);
	return CELL_OK;
}

s32 cellMusicInitializeSystemWorkload()
{
	UNIMPLEMENTED_FUNC(cellMusic);
	return CELL_OK;
}

s32 cellMusicInitialize()
{
	UNIMPLEMENTED_FUNC(cellMusic);
	return CELL_OK;
}

s32 cellMusicFinalize2()
{
	UNIMPLEMENTED_FUNC(cellMusic);
	return CELL_OK;
}

s32 cellMusicGetSelectionContext2()
{
	UNIMPLEMENTED_FUNC(cellMusic);
	return CELL_OK;
}

s32 cellMusicGetVolume()
{
	UNIMPLEMENTED_FUNC(cellMusic);
	return CELL_OK;
}

s32 cellMusicGetPlaybackStatus()
{
	UNIMPLEMENTED_FUNC(cellMusic);
	return CELL_OK;
}

s32 cellMusicSetPlaybackCommand2()
{
	UNIMPLEMENTED_FUNC(cellMusic);
	return CELL_OK;
}

s32 cellMusicSetPlaybackCommand()
{
	UNIMPLEMENTED_FUNC(cellMusic);
	return CELL_OK;
}

s32 cellMusicSelectContents2()
{
	UNIMPLEMENTED_FUNC(cellMusic);
	return CELL_OK;
}

s32 cellMusicSelectContents()
{
	UNIMPLEMENTED_FUNC(cellMusic);
	return CELL_OK;
}

s32 cellMusicInitialize2(s32 mode, s32 spuPriority, vm::ptr<CellMusic2Callback> func, vm::ptr<void> userData)
{
	cellMusic.todo("cellMusicInitialize2(mode=%d, spuPriority=%d, func=*0x%x, userData=*0x%x)", mode, spuPriority, func, userData);

	if (mode != CELL_MUSIC2_PLAYER_MODE_NORMAL)
	{
		cellMusic.todo("Unknown player mode: 0x%x", mode);
		return CELL_MUSIC2_ERROR_PARAM;
	}

	const auto music = fxm::make_always<music2_t>();
	music->func = func;
	music->userData = userData;

	sysutil_register_cb([=](ppu_thread& ppu) -> s32
	{
		func(ppu, CELL_MUSIC2_EVENT_INITIALIZE_RESULT, vm::make_var<s32>(CELL_OK), userData);
		return CELL_OK;
	});

	return CELL_OK;
}

s32 cellMusicSetVolume()
{
	UNIMPLEMENTED_FUNC(cellMusic);
	return CELL_OK;
}

s32 cellMusicGetVolume2()
{
	UNIMPLEMENTED_FUNC(cellMusic);
	return CELL_OK;
}


DECLARE(ppu_module_manager::cellMusic)("cellMusicUtility", []()
{
	REG_FUNC(cellMusicUtility, cellMusicGetSelectionContext);
	REG_FUNC(cellMusicUtility, cellMusicSetSelectionContext2);
	REG_FUNC(cellMusicUtility, cellMusicSetVolume2);
	REG_FUNC(cellMusicUtility, cellMusicGetContentsId);
	REG_FUNC(cellMusicUtility, cellMusicSetSelectionContext);
	REG_FUNC(cellMusicUtility, cellMusicInitialize2SystemWorkload);
	REG_FUNC(cellMusicUtility, cellMusicGetPlaybackStatus2);
	REG_FUNC(cellMusicUtility, cellMusicGetContentsId2);
	REG_FUNC(cellMusicUtility, cellMusicFinalize);
	REG_FUNC(cellMusicUtility, cellMusicInitializeSystemWorkload);
	REG_FUNC(cellMusicUtility, cellMusicInitialize);
	REG_FUNC(cellMusicUtility, cellMusicFinalize2);
	REG_FUNC(cellMusicUtility, cellMusicGetSelectionContext2);
	REG_FUNC(cellMusicUtility, cellMusicGetVolume);
	REG_FUNC(cellMusicUtility, cellMusicGetPlaybackStatus);
	REG_FUNC(cellMusicUtility, cellMusicSetPlaybackCommand2);
	REG_FUNC(cellMusicUtility, cellMusicSetPlaybackCommand);
	REG_FUNC(cellMusicUtility, cellMusicSelectContents2);
	REG_FUNC(cellMusicUtility, cellMusicSelectContents);
	REG_FUNC(cellMusicUtility, cellMusicInitialize2);
	REG_FUNC(cellMusicUtility, cellMusicSetVolume);
	REG_FUNC(cellMusicUtility, cellMusicGetVolume2);
});
