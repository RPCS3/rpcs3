#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"

#include "cellBgdl.h"

LOG_CHANNEL(cellBGDL);

error_code cellBGDLGetInfo(vm::cptr<char> content_id, vm::ptr<CellBGDLInfo> info, s32 num)
{
	cellBGDL.todo("cellBGDLGetInfo(content_id=%s, info=*0x%x, num=%d)", content_id, info, num);
	return CELL_OK;
}

error_code cellBGDLGetInfo2(vm::cptr<char> service_id, vm::ptr<CellBGDLInfo> info, s32 num)
{
	cellBGDL.todo("cellBGDLGetInfo2(service_id=%s, info=*0x%x, num=%d)", service_id, info, num);
	return CELL_OK;
}

error_code cellBGDLSetMode(CellBGDLMode mode)
{
	cellBGDL.todo("cellBGDLSetMode(mode=%d)", +mode);
	return CELL_OK;
}

error_code cellBGDLGetMode(vm::ptr<CellBGDLMode> mode)
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
