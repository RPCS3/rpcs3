#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/Memory/vm.h"
#include "Emu/Cell/ErrorCodes.h"

#include "sys_process.h"
#include "sys_hid.h"

LOG_CHANNEL(sys_hid);

error_code sys_hid_manager_open(u64 device_type, u64 port_no, vm::ptr<u32> handle)
{
	sys_hid.todo("sys_hid_manager_open(device_type=0x%llx, port_no=0x%llx, handle=*0x%llx)", device_type, port_no, handle);

	return CELL_OK;
}

error_code sys_hid_manager_ioctl(u32 hid_handle, u32 pkg_id, vm::ptr<void> buf, u64 buf_size)
{
	sys_hid.todo("sys_hid_manager_ioctl(hid_handle=0x%x, pkg_id=0x%llx, buf=*0x%x, buf_size=0x%llx)", hid_handle, pkg_id, buf, buf_size);

	return CELL_OK;
}

error_code sys_hid_manager_check_focus()
{
	sys_hid.todo("sys_hid_manager_check_focus()");

	return CELL_OK;
}

error_code sys_hid_manager_514(u32 pkg_id, vm::ptr<void> buf, u64 buf_size)
{
	sys_hid.todo("sys_hid_manager_514(pkg_id=0x%x, buf=*0x%x, buf_size=0x%llx)", pkg_id, buf, buf_size);

	return CELL_OK;
}

error_code sys_hid_manager_is_process_permission_root(u32 pid)
{
	sys_hid.todo("sys_hid_manager_is_process_permission_root(pid=0x%x)", pid);

	return not_an_error(g_ps3_process_info.has_root_perm());
}

error_code sys_hid_manager_add_hot_key_observer(u32 event_queue, vm::ptr<u32> unk)
{
	sys_hid.todo("sys_hid_manager_add_hot_key_observer(event_queue=0x%x, unk=*0x%x)", event_queue, unk);

	return CELL_OK;
}

error_code sys_hid_manager_read(u32 handle, u32 pkg_id, vm::ptr<void> buf, u64 buf_size)
{
	sys_hid.todo("sys_hid_manager_read(handle=0x%x, pkg_id=0x%x, buf=*0x%x, buf_size=0x%llx)", handle, pkg_id, buf, buf_size);

	return CELL_OK;
}
