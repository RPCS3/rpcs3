#pragma once

#include "Emu/Memory/vm_ptr.h"
#include "Emu/Cell/ErrorCodes.h"

enum : u64
{
	SYS_GPIO_UNKNOWN_DEVICE_ID    = 0x0,
	SYS_GPIO_LED_DEVICE_ID        = 0x1,
	SYS_GPIO_DIP_SWITCH_DEVICE_ID = 0x2,
};

error_code sys_gpio_get(u64 device_id, vm::ptr<u64> value);
error_code sys_gpio_set(u64 device_id, u64 mask, u64 value);
