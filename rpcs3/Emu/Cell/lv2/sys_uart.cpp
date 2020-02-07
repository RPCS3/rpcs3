#include "stdafx.h"
#include "Emu/System.h"

#include "Emu/Cell/ErrorCodes.h"

#include "sys_uart.h"

LOG_CHANNEL(sys_uart);

error_code sys_uart_initialize()
{
	sys_uart.todo("sys_uart_initialize()");

	return CELL_OK;
}

error_code sys_uart_receive(vm::ptr<void> buffer, u64 size, u32 unk)
{
	sys_uart.todo("sys_uart_receive(buffer=*0x%x, size=0x%llx, unk=0x%x)", buffer, size, unk);

	return CELL_OK;
}

error_code sys_uart_send(vm::cptr<void> buffer, u64 size, u64 flags)
{
	sys_uart.todo("sys_uart_send(buffer=0x%x, size=0x%llx, flags=0x%x)", buffer, size, flags);

	return CELL_OK;
}

error_code sys_uart_get_params(vm::ptr<char> buffer)
{
	sys_uart.todo("sys_uart_get_params(buffer=0x%x)", buffer);

	return CELL_OK;
}
