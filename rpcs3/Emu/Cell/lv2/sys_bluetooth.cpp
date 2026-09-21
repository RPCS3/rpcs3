#include "stdafx.h"
#include "sys_sync.h"
#include "Emu/Cell/PPUThread.h"

#include "sys_bluetooth.h"


LOG_CHANNEL(sys_bluetooth);

template <>
void fmt_class_string<sysBluetoothError>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto error)
	{
		switch (error)
		{
		STR_CASE(SYS_BLUETOOTH_ERROR_NOSYS);
		}

		return unknown;
	});
}

error_code sys_bluetooth_aud_serial_get_event_579(ppu_thread& ppu, u32 arg_1, vm::ptr<u32> out1, vm::ptr<u32> out2, vm::ptr<u32> out3, vm::ptr<u32> out4)
{
	ppu.state += cpu_flag::wait;

	sys_bluetooth.trace("sys_bluetooth_aud_serial_get_event_579(arg_1=%d, out1=%s, out2=%s, out3=%s, out4=%s)", arg_1, out1, out2, out3, out4);

	if (!out1 || !out2 || !out3 || !out4)
	{
		return CELL_EINVAL;
	}

	if (arg_1 > 1)
	{
		return SYS_BLUETOOTH_ERROR_NOSYS;
	}

	return CELL_OK;
}
