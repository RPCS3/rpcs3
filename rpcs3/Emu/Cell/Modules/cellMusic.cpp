#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/IdManager.h"
#include "Emu/Cell/PPUModule.h"

#include "cellMusic.h"

logs::channel cellMusic("cellMusic", logs::level::notice);

struct music2_t
{
	vm::ptr<CellMusic2Callback> func;
	vm::ptr<void> userData;
};

s32 cellMusicGetSelectionContext()
{
	throw EXCEPTION("");
}

s32 cellMusicSetSelectionContext2()
{
	throw EXCEPTION("");
}

s32 cellMusicSetVolume2()
{
	throw EXCEPTION("");
}

s32 cellMusicGetContentsId()
{
	throw EXCEPTION("");
}

s32 cellMusicSetSelectionContext()
{
	throw EXCEPTION("");
}

s32 cellMusicInitialize2SystemWorkload()
{
	throw EXCEPTION("");
}

s32 cellMusicGetPlaybackStatus2()
{
	throw EXCEPTION("");
}

s32 cellMusicGetContentsId2()
{
	throw EXCEPTION("");
}

s32 cellMusicFinalize()
{
	throw EXCEPTION("");
}

s32 cellMusicInitializeSystemWorkload()
{
	throw EXCEPTION("");
}

s32 cellMusicInitialize()
{
	throw EXCEPTION("");
}

s32 cellMusicFinalize2()
{
	throw EXCEPTION("");
}

s32 cellMusicGetSelectionContext2()
{
	throw EXCEPTION("");
}

s32 cellMusicGetVolume()
{
	throw EXCEPTION("");
}

s32 cellMusicGetPlaybackStatus()
{
	throw EXCEPTION("");
}

s32 cellMusicSetPlaybackCommand2()
{
	throw EXCEPTION("");
}

s32 cellMusicSetPlaybackCommand()
{
	throw EXCEPTION("");
}

s32 cellMusicSelectContents2()
{
	throw EXCEPTION("");
}

s32 cellMusicSelectContents()
{
	throw EXCEPTION("");
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

	Emu.GetCallbackManager().Register([=](PPUThread& ppu) -> s32
	{
		func(ppu, CELL_MUSIC2_EVENT_INITIALIZE_RESULT, vm::make_var<s32>(CELL_OK), userData);
		return CELL_OK;
	});

	return CELL_OK;
}

s32 cellMusicSetVolume()
{
	throw EXCEPTION("");
}

s32 cellMusicGetVolume2()
{
	throw EXCEPTION("");
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
