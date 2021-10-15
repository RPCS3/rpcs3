#include "stdafx.h"
#include "sys_gamepad.h"
#include "Emu/Cell/ErrorCodes.h"

LOG_CHANNEL(sys_gamepad);

u32 sys_gamepad_ycon_initalize(vm::ptr<u8> in, vm::ptr<u8> out)
{
	sys_gamepad.todo("sys_gamepad_ycon_initalize(in=%d, out=%d) -> CELL_OK", in, out);
	return CELL_OK;
}

u32 sys_gamepad_ycon_finalize(vm::ptr<u8> in, vm::ptr<u8> out)
{
	sys_gamepad.todo("sys_gamepad_ycon_finalize(in=%d, out=%d) -> CELL_OK", in, out);
	return CELL_OK;
}

u32 sys_gamepad_ycon_has_input_ownership(vm::ptr<u8> in, vm::ptr<u8> out)
{
	sys_gamepad.todo("sys_gamepad_ycon_has_input_ownership(in=%d, out=%d) -> CELL_OK", in, out);
	return CELL_OK;
}

u32 sys_gamepad_ycon_enumerate_device(vm::ptr<u8> in, vm::ptr<u8> out)
{
	sys_gamepad.todo("sys_gamepad_ycon_enumerate_device(in=%d, out=%d) -> CELL_OK", in, out);
	return CELL_OK;
}

u32 sys_gamepad_ycon_get_device_info(vm::ptr<u8> in, vm::ptr<u8> out)
{
	sys_gamepad.todo("sys_gamepad_ycon_get_device_info(in=%d, out=%d) -> CELL_OK", in, out);
	return CELL_OK;
}

u32 sys_gamepad_ycon_read_raw_report(vm::ptr<u8> in, vm::ptr<u8> out)
{
	sys_gamepad.todo("sys_gamepad_ycon_read_raw_report(in=%d, out=%d) -> CELL_OK", in, out);
	return CELL_OK;
}

u32 sys_gamepad_ycon_write_raw_report(vm::ptr<u8> in, vm::ptr<u8> out)
{
	sys_gamepad.todo("sys_gamepad_ycon_write_raw_report(in=%d, out=%d) -> CELL_OK", in, out);
	return CELL_OK;
}

u32 sys_gamepad_ycon_get_feature(vm::ptr<u8> in, vm::ptr<u8> out)
{
	sys_gamepad.todo("sys_gamepad_ycon_get_feature(in=%d, out=%d) -> CELL_OK", in, out);
	return CELL_OK;
}

u32 sys_gamepad_ycon_set_feature(vm::ptr<u8> in, vm::ptr<u8> out)
{
	sys_gamepad.todo("sys_gamepad_ycon_set_feature(in=%d, out=%d) -> CELL_OK", in, out);
	return CELL_OK;
}

u32 sys_gamepad_ycon_is_gem(vm::ptr<u8> in, vm::ptr<u8> out)
{
	sys_gamepad.todo("sys_gamepad_ycon_is_gem(in=%d, out=%d) -> CELL_OK", in, out);
	return CELL_OK;
}

// syscall(621,packet_id,u8 *in,u8 *out) Talk:LV2_Functions_and_Syscalls#Syscall_621_.280x26D.29 gamepad_if usage
u32 sys_gamepad_ycon_if(u8 packet_id, vm::ptr<u8> in, vm::ptr<u8> out)
{

	switch (packet_id)
	{
	case 0:
		return sys_gamepad_ycon_initalize(in, out);
	case 1:
		return sys_gamepad_ycon_finalize(in, out);
	case 2:
		return sys_gamepad_ycon_has_input_ownership(in, out);
	case 3:
		return sys_gamepad_ycon_enumerate_device(in, out);
	case 4:
		return sys_gamepad_ycon_get_device_info(in, out);
	case 5:
		return sys_gamepad_ycon_read_raw_report(in, out);
	case 6:
		return sys_gamepad_ycon_write_raw_report(in, out);
	case 7:
		return sys_gamepad_ycon_get_feature(in, out);
	case 8:
		return sys_gamepad_ycon_set_feature(in, out);
	case 9:
		return sys_gamepad_ycon_is_gem(in, out);
	default:
		sys_gamepad.error("sys_gamepad_ycon_if(packet_id=*%d, in=%d, out=%d), unknown packet id", packet_id, in, out);
		break;
	}

	return CELL_OK;
}
