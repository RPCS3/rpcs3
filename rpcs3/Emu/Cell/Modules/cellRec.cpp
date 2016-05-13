#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"

logs::channel cellRec("cellRec", logs::level::notice);

s32 cellRecOpen()
{
	throw EXCEPTION("");
}

s32 cellRecClose()
{
	throw EXCEPTION("");
}

s32 cellRecGetInfo()
{
	throw EXCEPTION("");
}

s32 cellRecStop()
{
	throw EXCEPTION("");
}

s32 cellRecStart()
{
	throw EXCEPTION("");
}

s32 cellRecQueryMemSize()
{
	throw EXCEPTION("");
}

s32 cellRecSetInfo()
{
	throw EXCEPTION("");
}


DECLARE(ppu_module_manager::cellRec)("cellRec", []()
{
	REG_FUNC(cellRec, cellRecOpen);
	REG_FUNC(cellRec, cellRecClose);
	REG_FUNC(cellRec, cellRecGetInfo);
	REG_FUNC(cellRec, cellRecStop);
	REG_FUNC(cellRec, cellRecStart);
	REG_FUNC(cellRec, cellRecQueryMemSize);
	REG_FUNC(cellRec, cellRecSetInfo);
});
