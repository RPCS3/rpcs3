#include "stdafx.h"
#include "sys_gpio.h"

#include "Emu/Cell/ErrorCodes.h"

LOG_CHANNEL(sys_gpio);

error_code sys_gpio_get(u64 device_id, vm::ptr<u64> value)
{
	sys_gpio.trace("sys_gpio_get(device_id=0x%llx, value=*0x%x)", device_id, value);

	if (device_id != SYS_GPIO_LED_DEVICE_ID && device_id != SYS_GPIO_DIP_SWITCH_DEVICE_ID)
	{
		return CELL_ESRCH;
	}

	// Retail consoles dont have LEDs or DIPs switches, hence always sets 0 in paramenter
	if (!value.try_write(0))
	{
		return CELL_EFAULT;
	}

	return CELL_OK;
}

error_code sys_gpio_set(u64 device_id, u64 mask, u64 value)
{
	sys_gpio.trace("sys_gpio_set(device_id=0x%llx, mask=0x%llx, value=0x%llx)", device_id, mask, value);

	// Retail consoles dont have LEDs or DIPs switches, hence the syscall can't modify devices's value
	switch (device_id)
	{
	case SYS_GPIO_LED_DEVICE_ID: return CELL_OK;
	case SYS_GPIO_DIP_SWITCH_DEVICE_ID: return CELL_EINVAL;
	}

	return CELL_ESRCH;
}
