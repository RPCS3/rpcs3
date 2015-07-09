#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/IdManager.h"
#include "Emu/SysCalls/Modules.h"

#include "sys_lv2dbg.h"

extern Module sys_lv2dbg;

s32 sys_dbg_read_ppu_thread_context(u64 id, vm::ptr<sys_dbg_ppu_thread_context_t> ppu_context)
{
	throw EXCEPTION("");
}

s32 sys_dbg_read_spu_thread_context(u32 id, vm::ptr<sys_dbg_spu_thread_context_t> spu_context)
{
	throw EXCEPTION("");
}

s32 sys_dbg_read_spu_thread_context2(u32 id, vm::ptr<sys_dbg_spu_thread_context2_t> spu_context)
{
	throw EXCEPTION("");
}

s32 sys_dbg_set_stacksize_ppu_exception_handler(u32 stacksize)
{
	throw EXCEPTION("");
}

s32 sys_dbg_initialize_ppu_exception_handler(s32 prio)
{
	throw EXCEPTION("");
}

s32 sys_dbg_finalize_ppu_exception_handler()
{
	throw EXCEPTION("");
}

s32 sys_dbg_register_ppu_exception_handler(vm::ptr<dbg_exception_handler_t> callback, u64 ctrl_flags)
{
	throw EXCEPTION("");
}

s32 sys_dbg_unregister_ppu_exception_handler()
{
	throw EXCEPTION("");
}

s32 sys_dbg_signal_to_ppu_exception_handler(u64 flags)
{
	throw EXCEPTION("");
}

s32 sys_dbg_get_mutex_information(u32 id, vm::ptr<sys_dbg_mutex_information_t> info)
{
	throw EXCEPTION("");
}

s32 sys_dbg_get_cond_information(u32 id, vm::ptr<sys_dbg_cond_information_t> info)
{
	throw EXCEPTION("");
}

s32 sys_dbg_get_rwlock_information(u32 id, vm::ptr<sys_dbg_rwlock_information_t> info)
{
	throw EXCEPTION("");
}

s32 sys_dbg_get_event_queue_information(u32 id, vm::ptr<sys_dbg_event_queue_information_t> info)
{
	throw EXCEPTION("");
}

s32 sys_dbg_get_semaphore_information(u32 id, vm::ptr<sys_dbg_semaphore_information_t> info)
{
	throw EXCEPTION("");
}

s32 sys_dbg_get_lwmutex_information(u32 id, vm::ptr<sys_dbg_lwmutex_information_t> info)
{
	throw EXCEPTION("");
}

s32 sys_dbg_get_lwcond_information(u32 id, vm::ptr<sys_dbg_lwcond_information_t> info)
{
	throw EXCEPTION("");
}

s32 sys_dbg_get_event_flag_information(u32 id, vm::ptr<sys_dbg_event_flag_information_t> info)
{
	throw EXCEPTION("");
}

s32 sys_dbg_get_ppu_thread_ids(vm::ptr<u64> ids, vm::ptr<u64> ids_num, vm::ptr<u64> all_ids_num)
{
	throw EXCEPTION("");
}

s32 sys_dbg_get_spu_thread_group_ids(vm::ptr<u32> ids, vm::ptr<u64> ids_num, vm::ptr<u64> all_ids_num)
{
	throw EXCEPTION("");
}

s32 sys_dbg_get_spu_thread_ids(u32 group_id, vm::ptr<u32> ids, vm::ptr<u64> ids_num, vm::ptr<u64> all_ids_num)
{
	throw EXCEPTION("");
}

s32 sys_dbg_get_ppu_thread_name(u64 id, vm::ptr<char> name)
{
	throw EXCEPTION("");
}

s32 sys_dbg_get_spu_thread_name(u32 id, vm::ptr<char> name)
{
	throw EXCEPTION("");
}

s32 sys_dbg_get_spu_thread_group_name(u32 id, vm::ptr<char> name)
{
	throw EXCEPTION("");
}


s32 sys_dbg_get_ppu_thread_status(u64 id, vm::ptr<u32> status)
{
	throw EXCEPTION("");
}

s32 sys_dbg_get_spu_thread_group_status(u32 id, vm::ptr<u32> status)
{
	throw EXCEPTION("");
}


s32 sys_dbg_enable_floating_point_enabled_exception(u64 id, u64 flags, u64 opt1, u64 opt2)
{
	throw EXCEPTION("");
}

s32 sys_dbg_disable_floating_point_enabled_exception(u64 id, u64 flags, u64 opt1, u64 opt2)
{
	throw EXCEPTION("");
}


s32 sys_dbg_vm_get_page_information(u32 addr, u32 num, vm::ptr<sys_vm_page_information_t> pageinfo)
{
	throw EXCEPTION("");
}


s32 sys_dbg_set_address_to_dabr(u64 addr, u64 ctrl_flag)
{
	throw EXCEPTION("");
}

s32 sys_dbg_get_address_from_dabr(vm::ptr<u64> addr, vm::ptr<u64> ctrl_flag)
{
	throw EXCEPTION("");
}


s32 sys_dbg_signal_to_coredump_handler(u64 data1, u64 data2, u64 data3)
{
	throw EXCEPTION("");
}


s32 sys_dbg_mat_set_condition(u32 addr, u64 cond)
{
	throw EXCEPTION("");
}

s32 sys_dbg_mat_get_condition(u32 addr, vm::ptr<u64> condp)
{
	throw EXCEPTION("");
}


s32 sys_dbg_get_coredump_params(vm::ptr<s32> param)
{
	throw EXCEPTION("");
}

s32 sys_dbg_set_mask_to_ppu_exception_handler(u64 mask, u64 flags)
{
	throw EXCEPTION("");
}

Module sys_lv2dbg("sys_lv2dbg", []
{
	REG_FUNC(sys_lv2dbg, sys_dbg_read_ppu_thread_context);
	REG_FUNC(sys_lv2dbg, sys_dbg_read_spu_thread_context);
	REG_FUNC(sys_lv2dbg, sys_dbg_read_spu_thread_context2);
	REG_FUNC(sys_lv2dbg, sys_dbg_set_stacksize_ppu_exception_handler);
	REG_FUNC(sys_lv2dbg, sys_dbg_initialize_ppu_exception_handler);
	REG_FUNC(sys_lv2dbg, sys_dbg_finalize_ppu_exception_handler);
	REG_FUNC(sys_lv2dbg, sys_dbg_register_ppu_exception_handler);
	REG_FUNC(sys_lv2dbg, sys_dbg_unregister_ppu_exception_handler);
	REG_FUNC(sys_lv2dbg, sys_dbg_signal_to_ppu_exception_handler);
	REG_FUNC(sys_lv2dbg, sys_dbg_get_mutex_information);
	REG_FUNC(sys_lv2dbg, sys_dbg_get_cond_information);
	REG_FUNC(sys_lv2dbg, sys_dbg_get_rwlock_information);
	REG_FUNC(sys_lv2dbg, sys_dbg_get_event_queue_information);
	REG_FUNC(sys_lv2dbg, sys_dbg_get_semaphore_information);
	REG_FUNC(sys_lv2dbg, sys_dbg_get_lwmutex_information);
	REG_FUNC(sys_lv2dbg, sys_dbg_get_lwcond_information);
	REG_FUNC(sys_lv2dbg, sys_dbg_get_event_flag_information);
	REG_FUNC(sys_lv2dbg, sys_dbg_get_ppu_thread_ids);
	REG_FUNC(sys_lv2dbg, sys_dbg_get_spu_thread_group_ids);
	REG_FUNC(sys_lv2dbg, sys_dbg_get_spu_thread_ids);
	REG_FUNC(sys_lv2dbg, sys_dbg_get_ppu_thread_name);
	REG_FUNC(sys_lv2dbg, sys_dbg_get_spu_thread_name);
	REG_FUNC(sys_lv2dbg, sys_dbg_get_spu_thread_group_name);
	REG_FUNC(sys_lv2dbg, sys_dbg_get_ppu_thread_status);
	REG_FUNC(sys_lv2dbg, sys_dbg_get_spu_thread_group_status);
	REG_FUNC(sys_lv2dbg, sys_dbg_enable_floating_point_enabled_exception);
	REG_FUNC(sys_lv2dbg, sys_dbg_disable_floating_point_enabled_exception);
	REG_FUNC(sys_lv2dbg, sys_dbg_vm_get_page_information);
	REG_FUNC(sys_lv2dbg, sys_dbg_set_address_to_dabr);
	REG_FUNC(sys_lv2dbg, sys_dbg_get_address_from_dabr);
	REG_FUNC(sys_lv2dbg, sys_dbg_signal_to_coredump_handler);
	REG_FUNC(sys_lv2dbg, sys_dbg_mat_set_condition);
	REG_FUNC(sys_lv2dbg, sys_dbg_mat_get_condition);
	REG_FUNC(sys_lv2dbg, sys_dbg_get_coredump_params);
	REG_FUNC(sys_lv2dbg, sys_dbg_set_mask_to_ppu_exception_handler);
});
