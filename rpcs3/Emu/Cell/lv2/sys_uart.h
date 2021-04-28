#pragma once

#include "Emu/Memory/vm_ptr.h"

// SysCalls

error_code sys_uart_initialize();
error_code sys_uart_receive(vm::ptr<void> buffer, u64 size, u32 unk);
error_code sys_uart_send(vm::cptr<void> buffer, u64 size, u64 flags);
error_code sys_uart_get_params(vm::ptr<char> buffer);
