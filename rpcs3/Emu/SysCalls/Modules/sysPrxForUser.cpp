#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/SC_FUNC.h"

void sysPrxForUser_init();
Module sysPrxForUser("sysPrxForUser", sysPrxForUser_init);

void sys_initialize_tls()
{
	sysPrxForUser.Log("sys_initialize_tls()");
}

s64 sys_process_atexitspawn()
{
	sysPrxForUser.Log("sys_process_atexitspawn()");
	return 0;
}

s64 sys_process_at_Exitspawn()
{
	sysPrxForUser.Log("sys_process_at_Exitspawn");
	return 0;
}

int sys_process_is_stack(u32 p)
{
	PPCThread* CPU = GetCurrentPPCThread();
	if (p >= CPU->GetStackAddr() && p <= CPU->GetStackAddr() + CPU->GetStackSize())
		return 1;

	return 0;
}

int sys_spu_printf_initialize(int a1, int a2, int a3, int a4, int a5)
{
	sysPrxForUser.Warning("sys_spu_printf_initialize(0x%x, 0x%x, 0x%x, 0x%x, 0x%x)", a1, a2, a3, a4, a5);
	return 0;
}

s64 sys_prx_register_library(u32 lib_addr)
{
	sysPrxForUser.Error("sys_prx_register_library(lib_addr=0x%x)", lib_addr);
	return 0;
}

s64 sys_prx_exitspawn_with_level()
{
	sysPrxForUser.Log("sys_prx_exitspawn_with_level()");
	return 0;
}

s64 sys_strlen(u32 addr)
{
	const wxString& str = Memory.ReadString(addr);
	sysPrxForUser.Log("sys_strlen(0x%x - \"%s\")", addr, str.mb_str());
	return str.Len();
}

void sysPrxForUser_init()
{
	sysPrxForUser.AddFunc(0x744680a2, sys_initialize_tls);
	
	sysPrxForUser.AddFunc(0x2f85c0ef, sys_lwmutex_create);
	sysPrxForUser.AddFunc(0xc3476d0c, sys_lwmutex_destroy);
	sysPrxForUser.AddFunc(0x1573dc3f, sys_lwmutex_lock);
	sysPrxForUser.AddFunc(0xaeb78725, sys_lwmutex_trylock);
	sysPrxForUser.AddFunc(0x1bc200f4, sys_lwmutex_unlock);

	sysPrxForUser.AddFunc(0x8461e528, sys_time_get_system_time);

	sysPrxForUser.AddFunc(0xe6f2c1e7, sys_process_exit);
	sysPrxForUser.AddFunc(0x2c847572, sys_process_atexitspawn);
	sysPrxForUser.AddFunc(0x96328741, sys_process_at_Exitspawn);
	sysPrxForUser.AddFunc(0x4f7172c9, sys_process_is_stack);

	sysPrxForUser.AddFunc(0x24a1ea07, sys_ppu_thread_create);
	sysPrxForUser.AddFunc(0x350d454e, sys_ppu_thread_get_id);
	sysPrxForUser.AddFunc(0xaff080a4, sys_ppu_thread_exit);
	sysPrxForUser.AddFunc(0xa3e3be68, sys_ppu_thread_once);

	sysPrxForUser.AddFunc(0x45fe2fce, sys_spu_printf_initialize);

	sysPrxForUser.AddFunc(0x42b23552, sys_prx_register_library);
	sysPrxForUser.AddFunc(0xa2c7ba64, sys_prx_exitspawn_with_level);

	sysPrxForUser.AddFunc(0x2d36462b, sys_strlen);

	sysPrxForUser.AddFunc(0x35168520, sys_heap_malloc);
	//sysPrxForUser.AddFunc(0xaede4b03, sys_heap_free);
	//sysPrxForUser.AddFunc(0x8a561d92, sys_heap_delete_heap);
	sysPrxForUser.AddFunc(0xb2fcf2c8, sys_heap_create_heap);

	sysPrxForUser.AddFunc(0xb257540b, sys_mmapper_allocate_memory);
	sysPrxForUser.AddFunc(0xdc578057, sys_mmapper_map_memory);
}
