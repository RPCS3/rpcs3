#pragma once

#include "Emu/Memory/vm_ptr.h"
#include "Emu/Cell/ErrorCodes.h"

// SysCalls

error_code sys_bdemu_send_command(u64 cmd, u64 a2, u64 a3, vm::ptr<void> buf, u64 buf_len);
