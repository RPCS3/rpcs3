#pragma once

#include "Emu/Cell/ErrorCodes.h"

//Syscalls

error_code sys_gamepad_ycon_if(uint32_t packet_id, uint8_t *in, uint8_t *out);
