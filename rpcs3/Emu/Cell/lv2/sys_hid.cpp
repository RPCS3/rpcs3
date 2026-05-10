#include "stdafx.h"
#include "sys_hid.h"

#include "Emu/Memory/vm_var.h"

#include "Emu/Cell/PPUThread.h"
#include "Emu/Cell/ErrorCodes.h"
#include "Emu/Cell/Modules/cellPad.h"

#include "sys_process.h"

LOG_CHANNEL(sys_hid);

error_code sys_hid_manager_open(ppu_thread& ppu, u64 device_type, u64 port_no, vm::ptr<u32> handle)
{
	sys_hid.todo("sys_hid_manager_open(device_type=0x%llx, port_no=0x%llx, handle=*0x%llx)", device_type, port_no, handle);

	//device type == 1 = pad, 2 = kb, 3 = mouse
	if (device_type > 3)
	{
		return CELL_EINVAL;
	}

	if (!handle)
	{
		return CELL_EFAULT;
	}

	// 'handle' starts at 0x100 in realhw, and increments every time sys_hid_manager_open is called
	// however, sometimes the handle is reused when opening sys_hid_manager again (even when the previous one hasn't been closed yet) - maybe when processes/threads get killed/finish they also release their handles?
	static u32 ctr = 0x100;
	*handle = ctr++;

	if (device_type == 1)
	{
		cellPadInit(ppu, 7);
		cellPadSetPortSetting(::narrow<u32>(port_no) /* 0 */, CELL_PAD_SETTING_LDD | CELL_PAD_SETTING_PRESS_ON | CELL_PAD_SETTING_SENSOR_ON);
	}

	return CELL_OK;
}

error_code sys_hid_manager_ioctl(u32 hid_handle, u32 pkg_id, vm::ptr<void> buf, u64 buf_size)
{
	sys_hid.todo("sys_hid_manager_ioctl(hid_handle=0x%x, pkg_id=0x%llx, buf=*0x%x, buf_size=0x%llx)", hid_handle, pkg_id, buf, buf_size);

	// From realhw syscall dump when vsh boots
	// SC count | handle | pkg_id | *buf (in)                                                                 | *buf (out)                                                                | size -> ret
	// ---------|--------|--------|---------------------------------------------------------------------------|---------------------------------------------------------------------------|------------
	//    28893 |  0x101 |    0x2 | 000000000000000000000000000000000000000000                                | 054c02680102020000000000000008035000001c1f                                |   21 -> 0
	//    28894 |  0x101 |    0x3 | 00000000                                                                  | 00000000                                                                  |    4 -> 0
	//    28895 |  0x101 |    0x5 | 00000000                                                                  | 00000000                                                                  |    4 -> 0
	//    28896 |  0x101 |   0x68 | 01000000d0031cb020169e502006b7f80000000000606098000000000000000000000000d | 01000000d0031cb020169e502006b7f80000000000606098000000000000000000000000d |   64 -> 0
	//          |        |        | 0031c90000000002006bac400000000d0031cb0000000002006b4d0                   | 0031c90000000002006bac400000000d0031cb0000000002006b4d0                   |
	//    28898 |  0x102 |    0x2 | 000000000000000000000000000000000000000000                                | 054c02680102020000000000000008035000001c1f                                |   21 -> 0
	//    28901 |  0x100 |   0x64 | 00000001                                                                  | 00000001                                                                  |    4 -> 0xffffffff80010002  # x3::hidportassign
	//    2890  |  0x100 |   0x65 | 6b49d200                                                                  | 6b49d200                                                                  |    4 -> 0xffffffff80010002  # x3::hidportassign
	//    28903 |  0x100 |   0x66 | 00000001                                                                  | 00000001                                                                  |    4 -> 0  # x3::hidportassign
	//    28904 |  0x100 |    0x0 | 00000001000000ff000000ff000000ff000000ff000000010000000100000001000000010 | 00000001000000ff000000ff000000ff000000ff000000010000000100000001000000010 |   68 -> 0  # x3::hidportassign
	//          |        |        | 000000000000000000000000000000000000001000000010000000100000001           | 000000000000000000000000000000000000001000000010000000100000001           |
	//    28907 |  0x101 |    0x3 | 00000001                                                                  | 00000001                                                                  |    4 -> 0
	//    28908 |  0x101 |    0x5 | 00000001                                                                  | 00000001                                                                  |    4 -> 0
	//    29404 |  0x100 |    0x4 | 00                                                                        | ee                                                                        |    1 -> 0
	// *** repeats 30600, 31838, 33034, 34233, 35075 (35075 is x3::hidportassign) ***
	//    35076 |  0x100 |    0x0 | 00000001000000ff000000ff000000ff000000ff000000320000003200000032000000320 | 00000001000000ff000000ff000000ff000000ff000000320000003200000032000000320 |   68 -> 0
	//          |        |        | 000003200000032000000320000003200002710000027100000271000002710           | 000003200000032000000320000003200002710000027100000271000002710           |
	// *** more 0x4 that have buf(in)=00 and buf(out)=ee ***

	if (pkg_id == 2)
	{
		// Return what realhw seems to return
		// TODO: Figure out what this corresponds to
		auto info = vm::static_ptr_cast<sys_hid_info_2>(buf);
		info->vid = 0x054C;
		info->pid = 0x0268;

		u8 realhw[17] = { 0x01, 0x02, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x03, 0x50, 0x00, 0x00, 0x1c, 0x1f };
		memcpy(info->unk, &realhw, 17);
	}
	else if (pkg_id == 5)
	{
		auto info = vm::static_ptr_cast<sys_hid_info_5>(buf);
		info->vid = 0x054C;
		info->pid = 0x0268;
	}
	// pkg_id == 6 == setpressmode?
	else if (pkg_id == 0x68)
	{
		[[maybe_unused]] auto info = vm::static_ptr_cast<sys_hid_ioctl_68>(buf);
		//info->unk2 = 0;
	}

	return CELL_OK;
}

error_code sys_hid_manager_check_focus()
{
	// spammy sys_hid.todo("sys_hid_manager_check_focus()");

	return not_an_error(1);
}

error_code sys_hid_manager_513(u64 a1, u64 a2, vm::ptr<void> buf, u64 buf_size)
{
	sys_hid.todo("sys_hid_manager_513(%llx, %llx, buf=%llx, buf_size=%llx)", a1, a2, buf, buf_size);

	return CELL_OK;
}

error_code sys_hid_manager_514(u32 pkg_id, vm::ptr<void> buf, u64 buf_size)
{
	if (pkg_id == 0xE)
	{
		sys_hid.trace("sys_hid_manager_514(pkg_id=0x%x, buf=*0x%x, buf_size=0x%llx)", pkg_id, buf, buf_size);
	}
	else
	{
		sys_hid.todo("sys_hid_manager_514(pkg_id=0x%x, buf=*0x%x, buf_size=0x%llx)", pkg_id, buf, buf_size);
	}

	if (pkg_id == 0xE)
	{
		// buf holds device_type
		// auto device_type = vm::static_ptr_cast<u8>(buf);
		// spammy sys_hid.todo("device_type: 0x%x", device_type[0]);

		// return 1 or 0? look like almost like another check_focus type check, returning 0 looks to keep system focus
	}
	else if (pkg_id == 0xD)
	{
		auto inf = vm::static_ptr_cast<sys_hid_manager_514_pkg_d>(buf);
		// unk1 = (pad# << 24) | pad# | 0x100
		// return value doesn't seem to be used again
		sys_hid.todo("unk1: 0x%x, unk2:0x%x", inf->unk1, inf->unk2);
	}

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
	if (!buf)
	{
		return CELL_EFAULT;
	}

	(pkg_id == 2 || pkg_id == 0x81 ? sys_hid.trace : sys_hid.todo)
		("sys_hid_manager_read(handle=0x%x, pkg_id=0x%x, buf=*0x%x, buf_size=0x%llx)", handle, pkg_id, buf, buf_size);

	if (pkg_id == 2)
	{
		// cellPadGetData
		// it returns just button array from 'CellPadData'
		//auto data = vm::static_ptr_cast<u16[64]>(buf);
		// todo: use handle and dont call cellpad here
		vm::var<CellPadData> tmpData;
		if ((cellPadGetData(0, +tmpData) == CELL_OK) && tmpData->len > 0)
		{
			u64 cpySize = std::min(static_cast<u64>(tmpData->len) * sizeof(u16), buf_size * sizeof(u16));
			memcpy(buf.get_ptr(), &tmpData->button, cpySize);
			return not_an_error(cpySize);
		}
	}
	else if (pkg_id == 0x81)
	{
		// cellPadGetDataExtra?
		vm::var<CellPadData> tmpData;
		if ((cellPadGetData(0, +tmpData) == CELL_OK) && tmpData->len > 0)
		{
			u64 cpySize = std::min(static_cast<u64>(tmpData->len) * sizeof(u16), buf_size * sizeof(u16));
			memcpy(buf.get_ptr(), &tmpData->button, cpySize);
			return not_an_error(cpySize / 2);
		}
	}

	return CELL_OK;
}
