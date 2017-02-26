#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/IdManager.h"

#include "Emu/Cell/ErrorCodes.h"

#include "sys_gamepad.h"

namespace vm { using namespace ps3; }

logs::channel sys_gamepad("sys_gamepad", logs::level::notice);

// syscall(621,packet_id,uint8_t *in,uint8_t *out) Talk:LV2_Functions_and_Syscalls#Syscall_621_.280x26D.29 gamepad_if usage 
u32 sys_gamepad_ycon_if(uint8_t packet_id, vm::ps3::ptr<uint8_t> in, vm::ps3::ptr<uint8_t> out)
{
	sys_gamepad.error("sys_gamepad_ycon_if(packet_id=*%d, in=%d, out=%d)", packet_id, in, out);

	return CELL_OK;
}
