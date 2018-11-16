#include "stdafx.h"
#include "Emu/Memory/vm.h"
#include "Emu/System.h"
#include "Emu/Cell/PPUThread.h"

#include "Emu/Cell/ErrorCodes.h"
#include "Emu/Io/PadHandler.h"
#include "Emu/Cell/Modules/cellPad.h"

#include "sys_process.h"
#include "sys_hid.h"


LOG_CHANNEL(sys_hid);

error_code sys_hid_manager_open(u64 device_type, u64 port_no, vm::ptr<u32> handle)
{
	sys_hid.todo("sys_hid_manager_open(device_type=0x%llx, port_no=0x%llx, handle=*0x%llx)", device_type, port_no, handle);

	//device type == 1 = pad, 2 = kb, 3 = mouse
	if (device_type > 3)
	{
		return CELL_EINVAL;
	}

	// 'handle' might actually be some sort of port number, but may not actually matter for the time being
	static u32 ctr = 0x13370000;
	*handle = ctr++;

	//const auto handler = fxm::import<pad_thread>(Emu.GetCallbacks().get_pad_handler);
	cellPadInit(7);
	cellPadSetPortSetting(0, 1 | 2);

	return CELL_OK;
}

error_code sys_hid_manager_ioctl(u32 hid_handle, u32 pkg_id, vm::ptr<void> buf, u64 buf_size)
{
	sys_hid.todo("sys_hid_manager_ioctl(hid_handle=0x%x, pkg_id=0x%llx, buf=*0x%x, buf_size=0x%llx)", hid_handle, pkg_id, buf, buf_size);

	if (pkg_id == 5)
	{
		auto info = vm::static_ptr_cast<sys_hid_info_5>(buf);
		info->vid = 0x054C;
		info->pid = 0x0268;
		info->status = 2; // apparently 2 is correct, 0/1 doesnt cause vsh to call read
		// this is probly related to usbd, where the status is 'claimed' by kernel
	}
	else if (pkg_id == 2)
	{
		auto data = vm::static_ptr_cast<sys_hid_info_2>(buf);
		// puting all of these to 0xff reports full hid device capabilities
		// todo: figure out what each one cooresponds to
		data->unk1 = 0xFF;
		data->unk2 = 0xFF;
		data->unk3 = 0xFF;
		data->unk4 = 0xFF;
		data->unk5 = 0xFF;
		data->unk6 = 0xFF;
	}
	// pkg_id == 6 == setpressmode?
	else if (pkg_id == 0x68)
	{
		auto info = vm::static_ptr_cast<sys_hid_ioctl_68>(buf);
		//info->unk2 = 0;
	}

	return CELL_OK;
}

error_code sys_hid_manager_check_focus()
{
	sys_hid.todo("sys_hid_manager_check_focus()");

	return not_an_error(1);
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
		//auto device_type = vm::static_ptr_cast<u8>(buf);

		// return 1 or 0? look like almost like another check_focus type check, returning 0 looks to keep system focus

	}
	else if (pkg_id == 0xd)
	{
		auto inf = vm::static_ptr_cast<sys_hid_manager_514_pkg_d>(buf);
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
	if (pkg_id == 2 || pkg_id == 0x81)
	{
		sys_hid.trace("sys_hid_manager_read(handle=0x%x, pkg_id=0x%x, buf=*0x%x, buf_size=0x%llx)", handle, pkg_id, buf, buf_size);
	}
	else
	{
		sys_hid.todo("sys_hid_manager_read(handle=0x%x, pkg_id=0x%x, buf=*0x%x, buf_size=0x%llx)", handle, pkg_id, buf, buf_size);
	}

	if (pkg_id == 2) {
		// cellPadGetData
		// it returns just button array from 'CellPadData'
		//auto data = vm::static_ptr_cast<u16[64]>(buf);
		// todo: use handle and dont call cellpad here
		vm::var<CellPadData> tmpData;
		if ((cellPadGetData(0, tmpData) == CELL_OK) && tmpData->len > 0)
		{
			u64 cpySize = std::min((u64)tmpData->len * 2, buf_size * 2);
			memcpy(buf.get_ptr(), &tmpData->button, cpySize);
			return not_an_error(cpySize);
		}
	}
	else if (pkg_id == 0x81) {
		// cellPadGetDataExtra?
		vm::var<CellPadData> tmpData;
		cellPadGetData(0, tmpData);
		u64 cpySize = std::min((u64)tmpData->len * 2, buf_size);
		memcpy(buf.get_ptr(), &tmpData->button, cpySize);
		return not_an_error(cpySize / 2);
	}
	else if (pkg_id == 0xFF) {

	}

	return CELL_OK;
}
