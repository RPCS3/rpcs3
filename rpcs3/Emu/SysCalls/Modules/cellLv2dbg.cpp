#include "stdafx.h"
#if 0

void cellLv2dbg_init();
Module cellLv2dbg(0x002e, cellLv2dbg_init);

// Return Codes
enum
{
	CELL_LV2DBG_ERROR_DEINVALIDARGUMENTS = 0x80010409,
	CELL_LV2DBG_ERROR_DEOPERATIONDENIED  = 0x8001042c,
};

int sys_dbg_read_spu_thread_context()
{
	UNIMPLEMENTED_FUNC(cellLv2dbg);
	return CELL_OK;
}

int sys_dbg_initialize_ppu_exception_handler()
{
	UNIMPLEMENTED_FUNC(cellLv2dbg);
	return CELL_OK;
}

int sys_dbg_register_ppu_exception_handler()
{
	UNIMPLEMENTED_FUNC(cellLv2dbg);
	return CELL_OK;
}

int sys_dbg_finalize_ppu_exception_handler()
{
	UNIMPLEMENTED_FUNC(cellLv2dbg);
	return CELL_OK;
}

int sys_dbg_unregister_ppu_exception_handler()
{
	UNIMPLEMENTED_FUNC(cellLv2dbg);
	return CELL_OK;
}

int sys_dbg_set_stacksize_ppu_exception_handler()
{
	UNIMPLEMENTED_FUNC(cellLv2dbg);
	return CELL_OK;
}

int sys_dbg_signal_to_ppu_exception_handler()
{
	UNIMPLEMENTED_FUNC(cellLv2dbg);
	return CELL_OK;
}

int sys_dbg_enable_floating_point_enabled_exception()
{
	UNIMPLEMENTED_FUNC(cellLv2dbg);
	return CELL_OK;
}

int sys_dbg_disable_floating_point_enabled_exception()
{
	UNIMPLEMENTED_FUNC(cellLv2dbg);
	return CELL_OK;
}

int sys_dbg_set_address_to_dabr()
{
	UNIMPLEMENTED_FUNC(cellLv2dbg);
	return CELL_OK;
}

int sys_dbg_get_address_from_dabr()
{
	UNIMPLEMENTED_FUNC(cellLv2dbg);
	return CELL_OK;
}

int sys_dbg_set_mask_to_ppu_exception_handler()
{
	UNIMPLEMENTED_FUNC(cellLv2dbg);
	return CELL_OK;
}

int sys_dbg_read_ppu_thread_context()
{
	UNIMPLEMENTED_FUNC(cellLv2dbg);
	return CELL_OK;
}

int sys_dbg_read_spu_thread_context2()
{
	UNIMPLEMENTED_FUNC(cellLv2dbg);
	return CELL_OK;
}

int sys_dbg_get_ppu_thread_name()
{
	UNIMPLEMENTED_FUNC(cellLv2dbg);
	return CELL_OK;
}

int sys_dbg_get_spu_thread_name()
{
	UNIMPLEMENTED_FUNC(cellLv2dbg);
	return CELL_OK;
}

int sys_dbg_get_spu_thread_group_name()
{
	UNIMPLEMENTED_FUNC(cellLv2dbg);
	return CELL_OK;
}

int sys_dbg_get_ppu_thread_status()
{
	UNIMPLEMENTED_FUNC(cellLv2dbg);
	return CELL_OK;
}

int sys_dbg_get_spu_thread_group_status()
{
	UNIMPLEMENTED_FUNC(cellLv2dbg);
	return CELL_OK;
}

int sys_dbg_get_ppu_thread_ids()
{
	UNIMPLEMENTED_FUNC(cellLv2dbg);
	return CELL_OK;
}

int sys_dbg_get_spu_thread_ids()
{
	UNIMPLEMENTED_FUNC(cellLv2dbg);
	return CELL_OK;
}

int sys_dbg_get_spu_thread_group_ids()
{
	UNIMPLEMENTED_FUNC(cellLv2dbg);
	return CELL_OK;
}

int sys_dbg_get_mutex_information()
{
	UNIMPLEMENTED_FUNC(cellLv2dbg);
	return CELL_OK;
}

int sys_dbg_get_lwmutex_information()
{
	UNIMPLEMENTED_FUNC(cellLv2dbg);
	return CELL_OK;
}

int sys_dbg_get_rwlock_information()
{
	UNIMPLEMENTED_FUNC(cellLv2dbg);
	return CELL_OK;
}

int sys_dbg_get_semaphore_information()
{
	UNIMPLEMENTED_FUNC(cellLv2dbg);
	return CELL_OK;
}

int sys_dbg_get_cond_information()
{
	UNIMPLEMENTED_FUNC(cellLv2dbg);
	return CELL_OK;
}

int sys_dbg_get_lwcond_information()
{
	UNIMPLEMENTED_FUNC(cellLv2dbg);
	return CELL_OK;
}

int sys_dbg_get_event_queue_information()
{
	UNIMPLEMENTED_FUNC(cellLv2dbg);
	return CELL_OK;
}

int sys_dbg_get_event_flag_information()
{
	UNIMPLEMENTED_FUNC(cellLv2dbg);
	return CELL_OK;
}

int sys_dbg_vm_get_page_information()
{
	UNIMPLEMENTED_FUNC(cellLv2dbg);
	return CELL_OK;
}

int sys_dbg_mat_set_condition()
{
	UNIMPLEMENTED_FUNC(cellLv2dbg);
	return CELL_OK;
}

int sys_dbg_mat_get_condition()
{
	UNIMPLEMENTED_FUNC(cellLv2dbg);
	return CELL_OK;
}

int sys_dbg_signal_to_coredump_handler()
{
	UNIMPLEMENTED_FUNC(cellLv2dbg);
	return CELL_OK;
}

int sys_dbg_get_coredump_params()
{
	UNIMPLEMENTED_FUNC(cellLv2dbg);
	return CELL_OK;
}

void cellLv2dbg_init()
{
	REG_FUNC(cellLv2dbg, sys_dbg_read_spu_thread_context);
	REG_FUNC(cellLv2dbg, sys_dbg_initialize_ppu_exception_handler);
	REG_FUNC(cellLv2dbg, sys_dbg_register_ppu_exception_handler);
	REG_FUNC(cellLv2dbg, sys_dbg_finalize_ppu_exception_handler);
	REG_FUNC(cellLv2dbg, sys_dbg_unregister_ppu_exception_handler);
	REG_FUNC(cellLv2dbg, sys_dbg_set_stacksize_ppu_exception_handler);
	REG_FUNC(cellLv2dbg, sys_dbg_signal_to_ppu_exception_handler);
	REG_FUNC(cellLv2dbg, sys_dbg_enable_floating_point_enabled_exception);
	REG_FUNC(cellLv2dbg, sys_dbg_disable_floating_point_enabled_exception);
	REG_FUNC(cellLv2dbg, sys_dbg_set_address_to_dabr);
	REG_FUNC(cellLv2dbg, sys_dbg_get_address_from_dabr);
	REG_FUNC(cellLv2dbg, sys_dbg_set_mask_to_ppu_exception_handler);

	REG_FUNC(cellLv2dbg, sys_dbg_read_ppu_thread_context);
	REG_FUNC(cellLv2dbg, sys_dbg_read_spu_thread_context2);

	REG_FUNC(cellLv2dbg, sys_dbg_get_ppu_thread_name);
	REG_FUNC(cellLv2dbg, sys_dbg_get_spu_thread_name);
	REG_FUNC(cellLv2dbg, sys_dbg_get_spu_thread_group_name);
	REG_FUNC(cellLv2dbg, sys_dbg_get_ppu_thread_status);
	REG_FUNC(cellLv2dbg, sys_dbg_get_spu_thread_group_status);

	REG_FUNC(cellLv2dbg, sys_dbg_get_ppu_thread_ids);
	REG_FUNC(cellLv2dbg, sys_dbg_get_spu_thread_ids);
	REG_FUNC(cellLv2dbg, sys_dbg_get_spu_thread_group_ids);

	REG_FUNC(cellLv2dbg, sys_dbg_get_mutex_information);
	REG_FUNC(cellLv2dbg, sys_dbg_get_lwmutex_information);
	REG_FUNC(cellLv2dbg, sys_dbg_get_rwlock_information);
	REG_FUNC(cellLv2dbg, sys_dbg_get_semaphore_information);
	REG_FUNC(cellLv2dbg, sys_dbg_get_cond_information);
	REG_FUNC(cellLv2dbg, sys_dbg_get_lwcond_information);
	REG_FUNC(cellLv2dbg, sys_dbg_get_event_queue_information);
	REG_FUNC(cellLv2dbg, sys_dbg_get_event_flag_information);

	REG_FUNC(cellLv2dbg, sys_dbg_vm_get_page_information);

	REG_FUNC(cellLv2dbg, sys_dbg_mat_set_condition);
	REG_FUNC(cellLv2dbg, sys_dbg_mat_get_condition);

	REG_FUNC(cellLv2dbg, sys_dbg_signal_to_coredump_handler);
	REG_FUNC(cellLv2dbg, sys_dbg_get_coredump_params);
}
#endif
