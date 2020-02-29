#pragma once

#include "Emu/Memory/vm_ptr.h"

// SysCalls

error_code sys_hid_manager_open(u64 device_type, u64 port_no, vm::ptr<u32> handle);
error_code sys_hid_manager_ioctl(u32 hid_handle, u32 pkg_id, vm::ptr<void> buf, u64 buf_size);
error_code sys_hid_manager_add_hot_key_observer(u32 event_queue, vm::ptr<u32> unk);
error_code sys_hid_manager_check_focus();
error_code sys_hid_manager_is_process_permission_root(u32 pid);
error_code sys_hid_manager_514(u32 pkg_id, vm::ptr<void> buf, u64 buf_size);
error_code sys_hid_manager_read(u32 handle, u32 pkg_id, vm::ptr<void> buf, u64 buf_size);
