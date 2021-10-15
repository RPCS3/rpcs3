#include "stdafx.h"
#include "sys_dbg.h"

#include "Emu/Cell/ErrorCodes.h"
#include "Emu/Cell/Modules/sys_lv2dbg.h"

LOG_CHANNEL(sys_dbg);

error_code sys_dbg_read_process_memory(s32 pid, u32 address, u32 size, vm::ptr<void> data)
{
	sys_dbg.warning("sys_dbg_read_process_memory(pid=0x%x, address=0x%llx, size=0x%x, data=*0x%x)", pid, address, size, data);

	// Todo(TGEnigma): Process lookup (only 1 process exists right now)
	if (pid != 1)
	{
		return CELL_LV2DBG_ERROR_DEINVALIDARGUMENTS;
	}

	if (!size || !data)
	{
		return CELL_LV2DBG_ERROR_DEINVALIDARGUMENTS;
	}

	// Check if data destination is writable
	if (!vm::check_addr(data.addr(), vm::page_writable, size))
	{
		return CELL_EFAULT;
	}

	std::memcpy(data.get_ptr(), vm::get_super_ptr(address), size);

	return CELL_OK;
}

error_code sys_dbg_write_process_memory(s32 pid, u32 address, u32 size, vm::cptr<void> data)
{
	sys_dbg.warning("sys_dbg_write_process_memory(pid=0x%x, address=0x%llx, size=0x%x, data=*0x%x)", pid, address, size, data);

	// Todo(TGEnigma): Process lookup (only 1 process exists right now)
	if (pid != 1)
	{
		return CELL_LV2DBG_ERROR_DEINVALIDARGUMENTS;
	}

	if (!size || !data)
	{
		return CELL_LV2DBG_ERROR_DEINVALIDARGUMENTS;
	}

	// Check if data source is readable
	if (!vm::check_addr(data.addr(), vm::page_readable, size))
	{
		return CELL_EFAULT;
	}

	std::memcpy(vm::get_super_ptr(address), data.get_ptr(), size);

	return CELL_OK;
}
