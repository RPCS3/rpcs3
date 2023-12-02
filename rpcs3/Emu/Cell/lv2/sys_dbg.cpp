#include "stdafx.h"
#include "sys_dbg.h"

#include "Emu/Cell/ErrorCodes.h"

#include "Emu/Cell/PPUInterpreter.h"
#include "Emu/Cell/Modules/sys_lv2dbg.h"
#include "Emu/Memory/vm_locking.h"

#include "util/asm.hpp"

void ppu_register_function_at(u32 addr, u32 size, ppu_intrp_func_t ptr = nullptr);

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

	vm::writer_lock lock;

	// Check if data destination is writable
	if (!vm::check_addr(data.addr(), vm::page_writable, size))
	{
		return CELL_EFAULT;
	}

	// Check if the source is readable
	if (!vm::check_addr(address, vm::page_readable, size))
	{
		return CELL_EFAULT;
	}

	std::memmove(data.get_ptr(), vm::base(address), size);

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

	// Check destination (can be read-only actually)
	if (!vm::check_addr(address, vm::page_readable, size))
	{
		return CELL_EFAULT;
	}

	vm::writer_lock lock;

	// Again
	if (!vm::check_addr(data.addr(), vm::page_readable, size) || !vm::check_addr(address, vm::page_readable, size))
	{
		return CELL_EFAULT;
	}

	const u8* data_ptr = static_cast<const u8*>(data.get_ptr());

	if ((address >> 28) == 0xDu)
	{
		// Stack pages (4k pages is the exception here)
		std::memmove(vm::base(address), data_ptr, size);
		return CELL_OK;
	}

	const u32 end = address + size;

	for (u32 i = address, exec_update_size = 0; i < end;)
	{
		const u32 op_size = std::min<u32>(utils::align<u32>(i + 1, 0x10000), end) - i;

		const bool is_exec = vm::check_addr(i, vm::page_executable | vm::page_readable);

		if (is_exec)
		{
			exec_update_size += op_size;
			i += op_size;
		}

		if (!is_exec || i >= end)
		{
			// Commit executable data update
			// The read memory is also super ptr so memmove can work correctly on all implementations
			const u32 before_addr = i - exec_update_size;
			std::memmove(vm::get_super_ptr(before_addr), vm::get_super_ptr(data.addr() + (before_addr - address)), exec_update_size);
			ppu_register_function_at(before_addr, exec_update_size);
			exec_update_size = 0;

			if (i >= end)
			{
				break;
			}
		}

		if (!is_exec)
		{
			std::memmove(vm::base(i), data_ptr + (i - address), op_size);
			i += op_size;
		}
	}

	return CELL_OK;
}
