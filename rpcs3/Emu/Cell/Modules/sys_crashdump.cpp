#include "stdafx.h"
#include "sys_crashdump.h"
#include "Emu/Cell/PPUModule.h"

LOG_CHANNEL(sys_crashdump);

error_code sys_crash_dump_get_user_log_area(u8 index, vm::ptr<sys_crash_dump_log_area_info_t> entry)
{
	sys_crashdump.todo("sys_crash_dump_get_user_log_area(index=%d, entry=*0x%x)", index, entry);

	if (index > SYS_CRASH_DUMP_MAX_LOG_AREA || !entry)
	{
		return CELL_EINVAL;
	}

	return CELL_OK;
}

error_code sys_crash_dump_set_user_log_area(u8 index, vm::ptr<sys_crash_dump_log_area_info_t> new_entry)
{
	sys_crashdump.todo("sys_crash_dump_set_user_log_area(index=%d, new_entry=*0x%x)", index, new_entry);

	if (index > SYS_CRASH_DUMP_MAX_LOG_AREA || !new_entry)
	{
		return CELL_EINVAL;
	}

	return CELL_OK;
}

DECLARE(ppu_module_manager::sys_crashdump) ("sys_crashdump", []()
{
	REG_FUNC(sys_crashdump, sys_crash_dump_get_user_log_area);
	REG_FUNC(sys_crashdump, sys_crash_dump_set_user_log_area);
});
