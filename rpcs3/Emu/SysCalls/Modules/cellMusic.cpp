#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/SysCalls/Modules.h"

extern Module cellMusic;

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

s32 cellMusicInitialize2()
{
	throw EXCEPTION("");
}

s32 cellMusicSetVolume()
{
	throw EXCEPTION("");
}

s32 cellMusicGetVolume2()
{
	throw EXCEPTION("");
}


Module cellMusic("cellMusic", []()
{
	REG_FUNC(cellMusic, cellMusicGetSelectionContext);
	REG_FUNC(cellMusic, cellMusicSetSelectionContext2);
	REG_FUNC(cellMusic, cellMusicSetVolume2);
	REG_FUNC(cellMusic, cellMusicGetContentsId);
	REG_FUNC(cellMusic, cellMusicSetSelectionContext);
	REG_FUNC(cellMusic, cellMusicInitialize2SystemWorkload);
	REG_FUNC(cellMusic, cellMusicGetPlaybackStatus2);
	REG_FUNC(cellMusic, cellMusicGetContentsId2);
	REG_FUNC(cellMusic, cellMusicFinalize);
	REG_FUNC(cellMusic, cellMusicInitializeSystemWorkload);
	REG_FUNC(cellMusic, cellMusicInitialize);
	REG_FUNC(cellMusic, cellMusicFinalize2);
	REG_FUNC(cellMusic, cellMusicGetSelectionContext2);
	REG_FUNC(cellMusic, cellMusicGetVolume);
	REG_FUNC(cellMusic, cellMusicGetPlaybackStatus);
	REG_FUNC(cellMusic, cellMusicSetPlaybackCommand2);
	REG_FUNC(cellMusic, cellMusicSetPlaybackCommand);
	REG_FUNC(cellMusic, cellMusicSelectContents2);
	REG_FUNC(cellMusic, cellMusicSelectContents);
	REG_FUNC(cellMusic, cellMusicInitialize2);
	REG_FUNC(cellMusic, cellMusicSetVolume);
	REG_FUNC(cellMusic, cellMusicGetVolume2);
});
