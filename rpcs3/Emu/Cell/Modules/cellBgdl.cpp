#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/Cell/PPUModule.h"

#include "cellBgdl.h"



logs::channel cellBGDL("cellBGDL");

s32 cellBGDLGetInfo(vm::cptr<char> content_id, vm::ptr<CellBGDLInfo> info, s32 num)
{
	cellBGDL.todo("cellBGDLGetInfo(content_id=%s, info=*0x%x, num=%d)", content_id, info, num);
	return 0;
}

s32 cellBGDLGetInfo2(vm::cptr<char> service_id, vm::ptr<CellBGDLInfo> info, s32 num)
{
	cellBGDL.todo("cellBGDLGetInfo2(service_id=%s, info=*0x%x, num=%d)", service_id, info, num);
	return 0;
}

s32 cellBGDLSetMode(CellBGDLMode mode)
{
	cellBGDL.todo("cellBGDLSetMode(mode=%d)", (s32) mode);
	return CELL_OK;
}

s32 cellBGDLGetMode(vm::ptr<CellBGDLMode> mode)
{
	cellBGDL.todo("cellBGDLGetMode(mode=*0x%x)", mode);
	return CELL_OK;
}

DECLARE(ppu_module_manager::cellBGDL)("cellBGDLUtility", []()
{
	REG_FUNC(cellBGDLUtility, cellBGDLGetInfo);
	REG_FUNC(cellBGDLUtility, cellBGDLGetInfo2);
	REG_FUNC(cellBGDLUtility, cellBGDLSetMode);
	REG_FUNC(cellBGDLUtility, cellBGDLGetMode);
});
