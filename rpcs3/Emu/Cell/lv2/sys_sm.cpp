#include "stdafx.h"
#include "Emu/System.h"

#include "Emu/Cell/ErrorCodes.h"
#include "Emu/Cell/PPUThread.h"
#include "Emu/Cell/lv2/sys_process.h"

#include "sys_sm.h"


LOG_CHANNEL(sys_sm);

error_code sys_sm_get_params(vm::ptr<u8> a, vm::ptr<u8> b, vm::ptr<u32> c, vm::ptr<u64> d)
{
	sys_sm.todo("sys_sm_get_params(a=*0x%x, b=*0x%x, c=*0x%x, d=*0x%x)", a, b, c, d);

	if (a) *a = 0; else return CELL_EFAULT;
	if (b) *b = 0; else return CELL_EFAULT;
	if (c) *c = 0x200; else return CELL_EFAULT;
	if (d) *d = 7; else return CELL_EFAULT;

	return CELL_OK;
}

error_code sys_sm_get_ext_event2(vm::ptr<u64> a1, vm::ptr<u64> a2, vm::ptr<u64> a3, u64 a4)
{
	sys_sm.todo("sys_sm_get_ext_event2(a1=*0x%x, a2=*0x%x, a3=*0x%x, a4=*0x%x, a4=0x%xll", a1, a2, a3, a4);

	if (a4 != 0 && a4 != 1)
	{
		return CELL_EINVAL;
	}

	// a1 == 7 - 'console too hot, restart'
	// a2 looks to be used if a1 is either 5 or 3?
	// a3 looks to be ignored in vsh

	if (a1) *a1 = 0; else return CELL_EFAULT;
	if (a2) *a2 = 0; else return CELL_EFAULT;
	if (a3) *a3 = 0; else return CELL_EFAULT;

	// eagain for no event
	return not_an_error(CELL_EAGAIN);
}

error_code sys_sm_shutdown(ppu_thread& ppu, u16 op, vm::ptr<void> param, u64 size)
{
	ppu.state += cpu_flag::wait;

	sys_sm.success("sys_sm_shutdown(op=0x%x, param=*0x%x, size=0x%x)", op, param, size);

	if (!g_ps3_process_info.has_root_perm())
	{
		return CELL_ENOSYS;
	}

	switch (op)
	{
	case 0x100:
	case 0x1100:
	{
		sys_sm.success("Received shutdown request from application");
		_sys_process_exit(ppu, 0, 0, 0);
		break;
	}
	case 0x200:
	case 0x1200:
	{
		sys_sm.success("Received reboot request from application");
		lv2_exitspawn(ppu, Emu.argv, Emu.envp, Emu.data);
		break;
	}
	case 0x8201:
	case 0x8202:
	case 0x8204:
	{
		sys_sm.warning("Unsupported LPAR operation: 0x%x", op);
		return CELL_ENOTSUP;
	}
	default: return CELL_EINVAL;
	}

	return CELL_OK;
}

error_code sys_sm_set_shop_mode(s32 mode)
{
	sys_sm.todo("sys_sm_set_shop_mode(mode=0x%x)", mode);

	return CELL_OK;
}

error_code sys_sm_control_led(u8 led, u8 action)
{
	sys_sm.todo("sys_sm_control_led(led=0x%x, action=0x%x)", led, action);

	return CELL_OK;
}

error_code sys_sm_ring_buzzer(u64 packet, u64 a1, u64 a2)
{
	sys_sm.todo("sys_sm_ring_buzzer(packet=0x%x, a1=0x%x, a2=0x%x)", packet, a1, a2);

	return CELL_OK;
}
