#pragma once

#include "Emu/Memory/vm_ptr.h"

//Syscalls

u32 sys_gamepad_ycon_if(u8 packet_id, vm::ptr<u8> in, vm::ptr<u8> out);
