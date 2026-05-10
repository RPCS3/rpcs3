#pragma once

#include "Emu/Memory/vm_ptr.h"

// SysCalls

error_code sys_console_write(vm::cptr<char> buf, u32 len);
constexpr auto sys_console_write2 = sys_console_write;
