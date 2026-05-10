#pragma once

#include "Emu/Memory/vm_ptr.h"
#include "Emu/Cell/ErrorCodes.h"

// set sensor mode? also getinfo?
struct sys_hid_info_5
{
	le_t<u16> vid;
	le_t<u16> pid;
	u8 status;
	// todo: more in this, not sure what tho
};

struct sys_hid_info_2
{
	be_t<u16> vid;
	be_t<u16> pid;
	u8 unk[17];
};

struct sys_hid_ioctl_68
{
	u8 unk;
	u8 unk2;
};

// unk
struct sys_hid_manager_514_pkg_d
{
	be_t<u32> unk1;
	u8 unk2;
};

// SysCalls

error_code sys_hid_manager_open(ppu_thread& ppu, u64 device_type, u64 port_no, vm::ptr<u32> handle);
error_code sys_hid_manager_ioctl(u32 hid_handle, u32 pkg_id, vm::ptr<void> buf, u64 buf_size);
error_code sys_hid_manager_add_hot_key_observer(u32 event_queue, vm::ptr<u32> unk);
error_code sys_hid_manager_check_focus();
error_code sys_hid_manager_is_process_permission_root(u32 pid);
error_code sys_hid_manager_513(u64 a1, u64 a2, vm::ptr<void> buf, u64 buf_size);
error_code sys_hid_manager_514(u32 pkg_id, vm::ptr<void> buf, u64 buf_size);
error_code sys_hid_manager_read(u32 handle, u32 pkg_id, vm::ptr<void> buf, u64 buf_size);
