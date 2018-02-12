#pragma once

#include "Emu/Cell/ErrorCodes.h"

//Syscalls

u32 sys_gamepad_ycon_if(uint8_t packet_id, vm::ptr<uint8_t> in, vm::ptr<uint8_t> out);
