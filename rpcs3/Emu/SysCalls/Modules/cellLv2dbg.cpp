#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/SC_FUNC.h"

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
	cellLv2dbg.AddFunc(0xc21ee635, sys_dbg_read_spu_thread_context);
	cellLv2dbg.AddFunc(0xc353353a, sys_dbg_initialize_ppu_exception_handler);
	cellLv2dbg.AddFunc(0x22916f45, sys_dbg_register_ppu_exception_handler);
	cellLv2dbg.AddFunc(0xc0eb9266, sys_dbg_finalize_ppu_exception_handler);
	cellLv2dbg.AddFunc(0xc6d7ec13, sys_dbg_unregister_ppu_exception_handler);
	cellLv2dbg.AddFunc(0x06a840f5, sys_dbg_set_stacksize_ppu_exception_handler);
	cellLv2dbg.AddFunc(0x4ded9f6c, sys_dbg_signal_to_ppu_exception_handler);
	cellLv2dbg.AddFunc(0x3147c6ca, sys_dbg_enable_floating_point_enabled_exception);
	cellLv2dbg.AddFunc(0xf254768c, sys_dbg_disable_floating_point_enabled_exception);
	cellLv2dbg.AddFunc(0xdb14b37b, sys_dbg_set_address_to_dabr);
	cellLv2dbg.AddFunc(0xbb0ae221, sys_dbg_get_address_from_dabr);
	cellLv2dbg.AddFunc(0xab475d53, sys_dbg_set_mask_to_ppu_exception_handler);

	cellLv2dbg.AddFunc(0xc5eef17f, sys_dbg_read_ppu_thread_context);
	cellLv2dbg.AddFunc(0x266c2bd3, sys_dbg_read_spu_thread_context2);

	cellLv2dbg.AddFunc(0x4b55f456, sys_dbg_get_ppu_thread_name);
	cellLv2dbg.AddFunc(0x3e5eed36, sys_dbg_get_spu_thread_name);
	cellLv2dbg.AddFunc(0xbd69e584, sys_dbg_get_spu_thread_group_name);
	cellLv2dbg.AddFunc(0x6b413178, sys_dbg_get_ppu_thread_status);
	cellLv2dbg.AddFunc(0x9ddb9dc3, sys_dbg_get_spu_thread_group_status);

	cellLv2dbg.AddFunc(0x113b0bea, sys_dbg_get_ppu_thread_ids);
	cellLv2dbg.AddFunc(0x1860f909, sys_dbg_get_spu_thread_ids);
	cellLv2dbg.AddFunc(0x08ef08a9, sys_dbg_get_spu_thread_group_ids);

	cellLv2dbg.AddFunc(0x50453aa8, sys_dbg_get_mutex_information);
	cellLv2dbg.AddFunc(0xcb377e36, sys_dbg_get_lwmutex_information);
	cellLv2dbg.AddFunc(0x9794bb53, sys_dbg_get_rwlock_information);
	cellLv2dbg.AddFunc(0xa2d6cbd2, sys_dbg_get_semaphore_information);
	cellLv2dbg.AddFunc(0x63bd413e, sys_dbg_get_cond_information);
	cellLv2dbg.AddFunc(0x7bdadb01, sys_dbg_get_lwcond_information);
	cellLv2dbg.AddFunc(0x381ae33e, sys_dbg_get_event_queue_information);
	cellLv2dbg.AddFunc(0xdf856979, sys_dbg_get_event_flag_information);

	cellLv2dbg.AddFunc(0x580f8203, sys_dbg_vm_get_page_information);

	cellLv2dbg.AddFunc(0x24a3d413, sys_dbg_mat_set_condition);
	cellLv2dbg.AddFunc(0x590a276e, sys_dbg_mat_get_condition);

	cellLv2dbg.AddFunc(0xd830062a, sys_dbg_signal_to_coredump_handler);
	cellLv2dbg.AddFunc(0xb9da87d3, sys_dbg_get_coredump_params);
}