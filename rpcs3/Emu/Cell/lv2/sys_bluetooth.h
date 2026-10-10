#pragma once

#include "util/types.hpp"
#include "Emu/Memory/vm_ptr.h"
#include "Emu/Cell/ErrorCodes.h"

enum sysBluetoothError : u32
{
	SYS_BLUETOOTH_ERROR_NOSYS = 0x80111003,
};

// SysCalls

class ppu_thread;

error_code sys_bluetooth_aud_serial_get_event_579(ppu_thread& ppu, u32 arg_1, vm::ptr<u32> out1, vm::ptr<u32> out2, vm::ptr<u32> out3, vm::ptr<u32> out4);
