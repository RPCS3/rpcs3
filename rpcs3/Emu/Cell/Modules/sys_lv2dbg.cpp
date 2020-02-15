#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"

#include "sys_lv2dbg.h"

LOG_CHANNEL(sys_lv2dbg);

template <>
void fmt_class_string<CellLv2DbgError>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](CellLv2DbgError value)
	{
		switch (value)
		{
		STR_CASE(CELL_LV2DBG_ERROR_DEINVALIDPROCESSID);
		STR_CASE(CELL_LV2DBG_ERROR_DEINVALIDTHREADID);
		STR_CASE(CELL_LV2DBG_ERROR_DEILLEGALREGISTERTYPE);
		STR_CASE(CELL_LV2DBG_ERROR_DEILLEGALREGISTERNUMBER);
		STR_CASE(CELL_LV2DBG_ERROR_DEILLEGALTHREADSTATE);
		STR_CASE(CELL_LV2DBG_ERROR_DEINVALIDEFFECTIVEADDRESS);
		STR_CASE(CELL_LV2DBG_ERROR_DENOTFOUNDPROCESSID);
		STR_CASE(CELL_LV2DBG_ERROR_DENOMEM);
		STR_CASE(CELL_LV2DBG_ERROR_DEINVALIDARGUMENTS);
		STR_CASE(CELL_LV2DBG_ERROR_DENOTFOUNDFILE);
		STR_CASE(CELL_LV2DBG_ERROR_DEINVALIDFILETYPE);
		STR_CASE(CELL_LV2DBG_ERROR_DENOTFOUNDTHREADID);
		STR_CASE(CELL_LV2DBG_ERROR_DEINVALIDTHREADSTATUS);
		STR_CASE(CELL_LV2DBG_ERROR_DENOAVAILABLEPROCESSID);
		STR_CASE(CELL_LV2DBG_ERROR_DENOTFOUNDEVENTHANDLER);
		STR_CASE(CELL_LV2DBG_ERROR_DESPNOROOM);
		STR_CASE(CELL_LV2DBG_ERROR_DESPNOTFOUND);
		STR_CASE(CELL_LV2DBG_ERROR_DESPINPROCESS);
		STR_CASE(CELL_LV2DBG_ERROR_DEINVALIDPRIMARYSPUTHREADID);
		STR_CASE(CELL_LV2DBG_ERROR_DETHREADSTATEISNOTSTOPPED);
		STR_CASE(CELL_LV2DBG_ERROR_DEINVALIDTHREADTYPE);
		STR_CASE(CELL_LV2DBG_ERROR_DECONTINUEFAILED);
		STR_CASE(CELL_LV2DBG_ERROR_DESTOPFAILED);
		STR_CASE(CELL_LV2DBG_ERROR_DENOEXCEPTION);
		STR_CASE(CELL_LV2DBG_ERROR_DENOMOREEVENTQUE);
		STR_CASE(CELL_LV2DBG_ERROR_DEEVENTQUENOTCREATED);
		STR_CASE(CELL_LV2DBG_ERROR_DEEVENTQUEOVERFLOWED);
		STR_CASE(CELL_LV2DBG_ERROR_DENOTIMPLEMENTED);
		STR_CASE(CELL_LV2DBG_ERROR_DEQUENOTREGISTERED);
		STR_CASE(CELL_LV2DBG_ERROR_DENOMOREEVENTPROCESS);
		STR_CASE(CELL_LV2DBG_ERROR_DEPROCESSNOTREGISTERED);
		STR_CASE(CELL_LV2DBG_ERROR_DEEVENTDISCARDED);
		STR_CASE(CELL_LV2DBG_ERROR_DENOMORESYNCID);
		STR_CASE(CELL_LV2DBG_ERROR_DESYNCIDALREADYADDED);
		STR_CASE(CELL_LV2DBG_ERROR_DESYNCIDNOTFOUND);
		STR_CASE(CELL_LV2DBG_ERROR_DESYNCIDNOTACQUIRED);
		STR_CASE(CELL_LV2DBG_ERROR_DEPROCESSALREADYREGISTERED);
		STR_CASE(CELL_LV2DBG_ERROR_DEINVALIDLSADDRESS);
		STR_CASE(CELL_LV2DBG_ERROR_DEINVALIDOPERATION);
		STR_CASE(CELL_LV2DBG_ERROR_DEINVALIDMODULEID);
		STR_CASE(CELL_LV2DBG_ERROR_DEHANDLERALREADYREGISTERED);
		STR_CASE(CELL_LV2DBG_ERROR_DEINVALIDHANDLER);
		STR_CASE(CELL_LV2DBG_ERROR_DEHANDLENOTREGISTERED);
		STR_CASE(CELL_LV2DBG_ERROR_DEOPERATIONDENIED);
		STR_CASE(CELL_LV2DBG_ERROR_DEHANDLERNOTINITIALIZED);
		STR_CASE(CELL_LV2DBG_ERROR_DEHANDLERALREADYINITIALIZED);
		STR_CASE(CELL_LV2DBG_ERROR_DEILLEGALCOREDUMPPARAMETER);
		}

		return unknown;
	});
}

s32 sys_dbg_read_ppu_thread_context(u64 id, vm::ptr<sys_dbg_ppu_thread_context_t> ppu_context)
{
	sys_lv2dbg.todo("sys_dbg_read_ppu_thread_context()");
	return CELL_OK;
}

s32 sys_dbg_read_spu_thread_context(u32 id, vm::ptr<sys_dbg_spu_thread_context_t> spu_context)
{
	sys_lv2dbg.todo("sys_dbg_read_spu_thread_context()");
	return CELL_OK;
}

s32 sys_dbg_read_spu_thread_context2(u32 id, vm::ptr<sys_dbg_spu_thread_context2_t> spu_context)
{
	sys_lv2dbg.todo("sys_dbg_read_spu_thread_context2()");
	return CELL_OK;
}

s32 sys_dbg_set_stacksize_ppu_exception_handler(u32 stacksize)
{
	sys_lv2dbg.todo("sys_dbg_set_stacksize_ppu_exception_handler()");
	return CELL_OK;
}

s32 sys_dbg_initialize_ppu_exception_handler(s32 prio)
{
	sys_lv2dbg.todo("sys_dbg_initialize_ppu_exception_handler()");
	return CELL_OK;
}

s32 sys_dbg_finalize_ppu_exception_handler()
{
	sys_lv2dbg.todo("sys_dbg_finalize_ppu_exception_handler()");
	return CELL_OK;
}

s32 sys_dbg_register_ppu_exception_handler(vm::ptr<dbg_exception_handler_t> callback, u64 ctrl_flags)
{
	sys_lv2dbg.todo("sys_dbg_register_ppu_exception_handler()");
	return CELL_OK;
}

s32 sys_dbg_unregister_ppu_exception_handler()
{
	sys_lv2dbg.todo("sys_dbg_unregister_ppu_exception_handler()");
	return CELL_OK;
}

s32 sys_dbg_signal_to_ppu_exception_handler(u64 flags)
{
	sys_lv2dbg.todo("sys_dbg_signal_to_ppu_exception_handler()");
	return CELL_OK;
}

s32 sys_dbg_get_mutex_information(u32 id, vm::ptr<sys_dbg_mutex_information_t> info)
{
	sys_lv2dbg.todo("sys_dbg_get_mutex_information()");
	return CELL_OK;
}

s32 sys_dbg_get_cond_information(u32 id, vm::ptr<sys_dbg_cond_information_t> info)
{
	sys_lv2dbg.todo("sys_dbg_get_cond_information()");
	return CELL_OK;
}

s32 sys_dbg_get_rwlock_information(u32 id, vm::ptr<sys_dbg_rwlock_information_t> info)
{
	sys_lv2dbg.todo("sys_dbg_get_rwlock_information()");
	return CELL_OK;
}

s32 sys_dbg_get_event_queue_information(u32 id, vm::ptr<sys_dbg_event_queue_information_t> info)
{
	sys_lv2dbg.todo("sys_dbg_get_event_queue_information()");
	return CELL_OK;
}

s32 sys_dbg_get_semaphore_information(u32 id, vm::ptr<sys_dbg_semaphore_information_t> info)
{
	sys_lv2dbg.todo("sys_dbg_get_semaphore_information()");
	return CELL_OK;
}

s32 sys_dbg_get_lwmutex_information(u32 id, vm::ptr<sys_dbg_lwmutex_information_t> info)
{
	sys_lv2dbg.todo("sys_dbg_get_lwmutex_information()");
	return CELL_OK;
}

s32 sys_dbg_get_lwcond_information(u32 id, vm::ptr<sys_dbg_lwcond_information_t> info)
{
	sys_lv2dbg.todo("sys_dbg_get_lwcond_information()");
	return CELL_OK;
}

s32 sys_dbg_get_event_flag_information(u32 id, vm::ptr<sys_dbg_event_flag_information_t> info)
{
	sys_lv2dbg.todo("sys_dbg_get_event_flag_information()");
	return CELL_OK;
}

s32 sys_dbg_get_ppu_thread_ids(vm::ptr<u64> ids, vm::ptr<u64> ids_num, vm::ptr<u64> all_ids_num)
{
	sys_lv2dbg.todo("sys_dbg_get_ppu_thread_ids()");
	return CELL_OK;
}

s32 sys_dbg_get_spu_thread_group_ids(vm::ptr<u32> ids, vm::ptr<u64> ids_num, vm::ptr<u64> all_ids_num)
{
	sys_lv2dbg.todo("sys_dbg_get_spu_thread_group_ids()");
	return CELL_OK;
}

s32 sys_dbg_get_spu_thread_ids(u32 group_id, vm::ptr<u32> ids, vm::ptr<u64> ids_num, vm::ptr<u64> all_ids_num)
{
	sys_lv2dbg.todo("sys_dbg_get_spu_thread_ids()");
	return CELL_OK;
}

s32 sys_dbg_get_ppu_thread_name(u64 id, vm::ptr<char> name)
{
	sys_lv2dbg.todo("sys_dbg_get_ppu_thread_name()");
	return CELL_OK;
}

s32 sys_dbg_get_spu_thread_name(u32 id, vm::ptr<char> name)
{
	sys_lv2dbg.todo("sys_dbg_get_spu_thread_name()");
	return CELL_OK;
}

s32 sys_dbg_get_spu_thread_group_name(u32 id, vm::ptr<char> name)
{
	sys_lv2dbg.todo("sys_dbg_get_spu_thread_group_name()");
	return CELL_OK;
}


s32 sys_dbg_get_ppu_thread_status(u64 id, vm::ptr<u32> status)
{
	sys_lv2dbg.todo("sys_dbg_get_ppu_thread_status()");
	return CELL_OK;
}

s32 sys_dbg_get_spu_thread_group_status(u32 id, vm::ptr<u32> status)
{
	sys_lv2dbg.todo("sys_dbg_get_spu_thread_group_status()");
	return CELL_OK;
}


s32 sys_dbg_enable_floating_point_enabled_exception(u64 id, u64 flags, u64 opt1, u64 opt2)
{
	sys_lv2dbg.todo("sys_dbg_enable_floating_point_enabled_exception()");
	return CELL_OK;
}

s32 sys_dbg_disable_floating_point_enabled_exception(u64 id, u64 flags, u64 opt1, u64 opt2)
{
	sys_lv2dbg.todo("sys_dbg_disable_floating_point_enabled_exception()");
	return CELL_OK;
}


s32 sys_dbg_vm_get_page_information(u32 addr, u32 num, vm::ptr<sys_vm_page_information_t> pageinfo)
{
	sys_lv2dbg.todo("sys_dbg_vm_get_page_information()");
	return CELL_OK;
}


s32 sys_dbg_set_address_to_dabr(u64 addr, u64 ctrl_flag)
{
	sys_lv2dbg.todo("sys_dbg_set_address_to_dabr()");
	return CELL_OK;
}

s32 sys_dbg_get_address_from_dabr(vm::ptr<u64> addr, vm::ptr<u64> ctrl_flag)
{
	sys_lv2dbg.todo("sys_dbg_get_address_from_dabr()");
	return CELL_OK;
}


s32 sys_dbg_signal_to_coredump_handler(u64 data1, u64 data2, u64 data3)
{
	sys_lv2dbg.todo("sys_dbg_signal_to_coredump_handler()");
	return CELL_OK;
}


s32 sys_dbg_mat_set_condition(u32 addr, u64 cond)
{
	sys_lv2dbg.todo("sys_dbg_mat_set_condition()");
	return CELL_OK;
}

s32 sys_dbg_mat_get_condition(u32 addr, vm::ptr<u64> condp)
{
	sys_lv2dbg.todo("sys_dbg_mat_get_condition()");
	return CELL_OK;
}


s32 sys_dbg_get_coredump_params(vm::ptr<s32> param)
{
	sys_lv2dbg.todo("sys_dbg_get_coredump_params()");
	return CELL_OK;
}

s32 sys_dbg_set_mask_to_ppu_exception_handler(u64 mask, u64 flags)
{
	sys_lv2dbg.todo("sys_dbg_set_mask_to_ppu_exception_handler()");
	return CELL_OK;
}

DECLARE(ppu_module_manager::sys_lv2dbg)("sys_lv2dbg", []
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
