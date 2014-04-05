#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/SC_FUNC.h"

void cellMusicDecode_init();
Module cellMusicDecode(0x004f, cellMusicDecode_init);

// Return Codes
enum
{
	CELL_MUSIC_DECODE_OK                      = 0,
	CELL_MUSIC_DECODE_CANCELED                = 1,
	CELL_MUSIC_DECODE_DECODE_FINISHED         = 0x8002C101,
	CELL_MUSIC_DECODE_ERROR_PARAM             = 0x8002C102,
	CELL_MUSIC_DECODE_ERROR_BUSY              = 0x8002C103,
	CELL_MUSIC_DECODE_ERROR_NO_ACTIVE_CONTENT = 0x8002C104,
	CELL_MUSIC_DECODE_ERROR_NO_MATCH_FOUND    = 0x8002C105,
	CELL_MUSIC_DECODE_ERROR_INVALID_CONTEXT   = 0x8002C106,
	CELL_MUSIC_DECODE_ERROR_DECODE_FAILURE    = 0x8002C107,
	CELL_MUSIC_DECODE_ERROR_NO_MORE_CONTENT   = 0x8002C108,
	CELL_MUSIC_DECODE_DIALOG_OPEN             = 0x8002C109,
	CELL_MUSIC_DECODE_DIALOG_CLOSE            = 0x8002C10A,
	CELL_MUSIC_DECODE_ERROR_NO_LPCM_DATA      = 0x8002C10B,
	CELL_MUSIC_DECODE_NEXT_CONTENTS_READY     = 0x8002C10C,
	CELL_MUSIC_DECODE_ERROR_GENERIC           = 0x8002C1FF,
};

int cellMusicDecodeInitialize()
{
	UNIMPLEMENTED_FUNC(cellMusicDecode);
	return CELL_OK;
}

int cellMusicDecodeInitializeSystemWorkload()
{
	UNIMPLEMENTED_FUNC(cellMusicDecode);
	return CELL_OK;
}

int cellMusicDecodeFinalize()
{
	UNIMPLEMENTED_FUNC(cellMusicDecode);
	return CELL_OK;
}

int cellMusicDecodeSelectContents()
{
	UNIMPLEMENTED_FUNC(cellMusicDecode);
	return CELL_OK;
}

int cellMusicDecodeSetDecodeCommand()
{
	UNIMPLEMENTED_FUNC(cellMusicDecode);
	return CELL_OK;
}

int cellMusicDecodeGetDecodeStatus()
{
	UNIMPLEMENTED_FUNC(cellMusicDecode);
	return CELL_OK;
}

int cellMusicDecodeRead()
{
	UNIMPLEMENTED_FUNC(cellMusicDecode);
	return CELL_OK;
}

int cellMusicDecodeGetSelectionContext()
{
	UNIMPLEMENTED_FUNC(cellMusicDecode);
	return CELL_OK;
}

int cellMusicDecodeSetSelectionContext()
{
	UNIMPLEMENTED_FUNC(cellMusicDecode);
	return CELL_OK;
}

int cellMusicDecodeGetContentsId()
{
	UNIMPLEMENTED_FUNC(cellMusicDecode);
	return CELL_OK;
}

void cellMusicDecode_init()
{
	cellMusicDecode.AddFunc(0xd55dbc11, cellMusicDecodeInitialize);
	cellMusicDecode.AddFunc(0x84f154b2, cellMusicDecodeInitializeSystemWorkload);
	cellMusicDecode.AddFunc(0xa8615dc8, cellMusicDecodeFinalize);
	cellMusicDecode.AddFunc(0xf24cb963, cellMusicDecodeSelectContents);
	cellMusicDecode.AddFunc(0x066bb1cf, cellMusicDecodeSetDecodeCommand);
	cellMusicDecode.AddFunc(0x5af74c50, cellMusicDecodeGetDecodeStatus);
	cellMusicDecode.AddFunc(0xa881b744, cellMusicDecodeRead);
	cellMusicDecode.AddFunc(0xdbf70550, cellMusicDecodeGetSelectionContext);
	cellMusicDecode.AddFunc(0xb84f5c81, cellMusicDecodeSetSelectionContext);
	cellMusicDecode.AddFunc(0x58ab1999, cellMusicDecodeGetContentsId);
}