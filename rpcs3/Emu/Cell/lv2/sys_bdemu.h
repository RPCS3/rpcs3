#pragma once

#include "Emu/Memory/vm_ptr.h"

// SysCalls

#include "Emu/Memory/vm_ptr.h"
#include "sys_sync.h"

error_code sys_bdemu_send_command(u64 cmd, u64 a2, u64 a3, vm::ptr<void> buf, u64 buf_len);
