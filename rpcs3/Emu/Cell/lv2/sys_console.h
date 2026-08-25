#pragma once

#include "Emu/Memory/vm_ptr.h"
#include "Emu/Cell/ErrorCodes.h"

// SysCalls
class ppu_thread;

error_code sys_console_write(ppu_thread& ppu, vm::cptr<char> buf, u32 len);
constexpr auto sys_console_write2 = sys_console_write;
