#pragma once

// Syscalls

error_code sys_dbg_read_process_memory(s32 pid, u32 address, u32 size, vm::ptr<void> data);
error_code sys_dbg_write_process_memory(s32 pid, u32 address, u32 size, vm::cptr<void> data);
