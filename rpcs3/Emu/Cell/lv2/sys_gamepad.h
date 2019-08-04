#pragma once

#include "Emu/Cell/ErrorCodes.h"
#include "Emu/Memory/vm_ptr.h"

//Syscalls

u32 sys_gamepad_ycon_if(uint8_t packet_id, vm::ptr<uint8_t> in, vm::ptr<uint8_t> out);
