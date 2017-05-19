#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"

logs::channel cellRec("cellRec");

s32 cellRecOpen()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellRecClose()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellRecGetInfo()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellRecStop()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellRecStart()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellRecQueryMemSize()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellRecSetInfo()
{
	fmt::throw_exception("Unimplemented" HERE);
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
