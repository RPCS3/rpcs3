#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/SysCalls/Modules.h"

extern Module cellRec;

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


Module cellRec("cellRec", []()
{
	REG_FUNC(cellRec, cellRecOpen);
	REG_FUNC(cellRec, cellRecClose);
	REG_FUNC(cellRec, cellRecGetInfo);
	REG_FUNC(cellRec, cellRecStop);
	REG_FUNC(cellRec, cellRecStart);
	REG_FUNC(cellRec, cellRecQueryMemSize);
	REG_FUNC(cellRec, cellRecSetInfo);
});
