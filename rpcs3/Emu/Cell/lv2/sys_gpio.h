#pragma once

enum : u64
{
	SYS_GPIO_UNKNOWN_DEVICE_ID,
	SYS_GPIO_LED_DEVICE_ID,
	SYS_GPIO_DIP_SWITCH_DEVICE_ID,
};

error_code sys_gpio_get(u64 device_id, vm::ptr<u64> value);
error_code sys_gpio_set(u64 device_id, u64 mask, u64 value);
