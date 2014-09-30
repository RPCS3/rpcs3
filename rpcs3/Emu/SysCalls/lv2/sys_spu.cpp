#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/SysCalls/SysCalls.h"

#include "Emu/CPU/CPUThreadManager.h"
#include "Emu/Cell/RawSPUThread.h"
#include "Emu/FS/vfsStreamMemory.h"
#include "Emu/FS/vfsFile.h"
#include "Loader/ELF.h"
#include "sys_spu.h"

static SysCallBase sys_spu("sys_spu");

u32 LoadSpuImage(vfsStream& stream, u32& spu_ep)
{
	ELFLoader l(stream);
	l.LoadInfo();
	const u32 alloc_size = 256 * 1024;
	u32 spu_offset = (u32)Memory.MainMem.AllocAlign(alloc_size);
	l.LoadData(spu_offset);
	spu_ep = l.GetEntry();
	return spu_offset;
}

s32 spu_image_import(sys_spu_image& img, u32 src, u32 type)
{
	vfsStreamMemory f(src);
	u32 entry;
	u32 offset = LoadSpuImage(f, entry);

	img.type = SYS_SPU_IMAGE_TYPE_USER;
	img.entry_point = entry;
	img.addr = offset; // TODO: writing actual segment info
	img.nsegs = 1; // wrong value

	return CELL_OK;
}

s32 sys_spu_image_open(vm::ptr<sys_spu_image> img, vm::ptr<const char> path)
{
	sys_spu.Warning("sys_spu_image_open(img_addr=0x%x, path_addr=0x%x [%s])", img.addr(), path.addr(), path.get_ptr());

	vfsFile f(path.get_ptr());
	if(!f.IsOpened())
	{
		sys_spu.Error("sys_spu_image_open error: '%s' not found!", path.get_ptr());
		return CELL_ENOENT;
	}

	u32 entry;
	u32 offset = LoadSpuImage(f, entry);

	img->type = SYS_SPU_IMAGE_TYPE_USER;
	img->entry_point = entry;
	img->addr = offset; // TODO: writing actual segment info
	img->nsegs = 1; // wrong value

	return CELL_OK;
}

SPUThread* spu_thread_initialize(SpuGroupInfo* group, u32 spu_num, sys_spu_image& img, const std::string& name, u32 option, u64 a1, u64 a2, u64 a3, u64 a4, std::function<void(SPUThread&)> task)
{
	if (option)
	{
		sys_spu.Todo("Unsupported SPU Thread options (0x%x)", option);
	}

	u32 spu_ep = (u32)img.entry_point;
	// Copy SPU image:
	// TODO: use segment info
	u32 spu_offset = (u32)Memory.Alloc(256 * 1024, 4096);
	memcpy(vm::get_ptr<void>(spu_offset), vm::get_ptr<void>(img.addr), 256 * 1024);

	SPUThread& new_thread = static_cast<SPUThread&>(Emu.GetCPU().AddThread(CPU_THREAD_SPU));
	//initialize from new place:
	new_thread.SetOffset(spu_offset);
	new_thread.SetEntry(spu_ep);
	new_thread.SetName(name);
	new_thread.m_custom_task = task;
	new_thread.Run();
	new_thread.GPR[3] = u128::from64(0, a1);
	new_thread.GPR[4] = u128::from64(0, a2);
	new_thread.GPR[5] = u128::from64(0, a3);
	new_thread.GPR[6] = u128::from64(0, a4);

	const u32 id = new_thread.GetId();
	if (group) group->list[spu_num] = id;
	new_thread.group = group;

	sys_spu.Warning("*** New SPU Thread [%s] (ep=0x%x, opt=0x%x, a1=0x%llx, a2=0x%llx, a3=0x%llx, a4=0x%llx): id=%d",
		name.c_str(), spu_ep, option, a1, a2, a3, a4, id);
	return &new_thread;
}

s32 sys_spu_thread_initialize(vm::ptr<be_t<u32>> thread, u32 group, u32 spu_num, vm::ptr<sys_spu_image> img, vm::ptr<sys_spu_thread_attribute> attr, vm::ptr<sys_spu_thread_argument> arg)
{
	sys_spu.Warning("sys_spu_thread_initialize(thread_addr=0x%x, group=0x%x, spu_num=%d, img_addr=0x%x, attr_addr=0x%x, arg_addr=0x%x)",
		thread.addr(), group, spu_num, img.addr(), attr.addr(), arg.addr());

	SpuGroupInfo* group_info;
	if(!Emu.GetIdManager().GetIDData(group, group_info))
	{
		return CELL_ESRCH;
	}

	if(spu_num >= group_info->list.size())
	{
		return CELL_EINVAL;
	}
	
	if(group_info->list[spu_num])
	{
		return CELL_EBUSY;
	}

	*thread = spu_thread_initialize(
		group_info,
		spu_num,
		*img,
		attr->name ? std::string(attr->name.get_ptr(), attr->name_len) : "SPUThread",
		attr->option,
		arg->arg1,
		arg->arg2,
		arg->arg3,
		arg->arg4)->GetId();
	return CELL_OK;
}

s32 sys_spu_thread_set_argument(u32 id, vm::ptr<sys_spu_thread_argument> arg)
{
	sys_spu.Warning("sys_spu_thread_set_argument(id=%d, arg_addr=0x%x)", id, arg.addr());

	CPUThread* thr = Emu.GetCPU().GetThread(id);

	if(!thr || thr->GetType() != CPU_THREAD_SPU)
	{
		return CELL_ESRCH;
	}

	SPUThread& spu = *(SPUThread*)thr;

	spu.GPR[3] = u128::from64(0, arg->arg1);
	spu.GPR[4] = u128::from64(0, arg->arg2);
	spu.GPR[5] = u128::from64(0, arg->arg3);
	spu.GPR[6] = u128::from64(0, arg->arg4);

	return CELL_OK;
}

s32 sys_spu_thread_get_exit_status(u32 id, vm::ptr<be_t<u32>> status)
{
	sys_spu.Warning("sys_spu_thread_get_exit_status(id=%d, status_addr=0x%x)", id, status.addr());

	CPUThread* thr = Emu.GetCPU().GetThread(id);

	if(!thr || thr->GetType() != CPU_THREAD_SPU)
	{
		return CELL_ESRCH;
	}

	u32 res;
	if (!(*(SPUThread*)thr).SPU.Out_MBox.Pop(res) || !thr->IsStopped())
	{
		return CELL_ESTAT;
	}

	*status = res;
	return CELL_OK;
}

s32 sys_spu_thread_group_destroy(u32 id)
{
	sys_spu.Warning("sys_spu_thread_group_destroy(id=%d)", id);

	SpuGroupInfo* group_info;
	if(!Emu.GetIdManager().GetIDData(id, group_info))
	{
		return CELL_ESRCH;
	}

	//TODO: New method to check busy. and even maybe in other sys_spu_thread_group_ calls.

	//TODO: SPU_THREAD_GROUP lock may not be gracefully implemented now.
	//		But it could still be set using simple way?
	//Check the state it should be in NOT_INITIALIZED / INITIALIZED.
	if ((group_info->m_state != SPU_THREAD_GROUP_STATUS_INITIALIZED)
		&& (group_info->m_state != SPU_THREAD_GROUP_STATUS_NOT_INITIALIZED))
	{
		sys_spu.Error("sys_spu_thread_group_destroy(id=%d) is not in NOT_INITIALIZED / INITIALIZED, state=%d", id, group_info->m_state);
		return CELL_ESTAT;	//Indeed this should not be encountered. If program itself all right.
	}
	//SET BUSY


	for (u32 i = 0; i < group_info->list.size(); i++)
	{
		// TODO: disconnect all event ports
		CPUThread* t = Emu.GetCPU().GetThread(group_info->list[i]);
		if (t)
		{
			Memory.Free(((SPUThread*)t)->GetOffset());
			Emu.GetCPU().RemoveThread(group_info->list[i]);
		}
	}

	group_info->m_state = SPU_THREAD_GROUP_STATUS_UNKNOWN;
	//REMOVE BUSY

	Emu.GetIdManager().RemoveID(id);
	return CELL_OK;
}

s32 sys_spu_thread_group_start(u32 id)
{
	sys_spu.Warning("sys_spu_thread_group_start(id=%d)", id);

	SpuGroupInfo* group_info;
	if(!Emu.GetIdManager().GetIDData(id, group_info))
	{
		return CELL_ESRCH;
	}

	// TODO: check group state

	//Check for BUSY?

	//SET BUSY

	//Different from what i expected. Or else there would not be any with RUNNING.
	group_info->m_state = SPU_THREAD_GROUP_STATUS_READY;	//Added Group State
	//Notice: I can not know the action preformed below be following the definition, but left unchanged.

	for (u32 i = 0; i < group_info->list.size(); i++)
	{
		CPUThread* t = Emu.GetCPU().GetThread(group_info->list[i]);
		if (t)
		{
			t->Exec();
		}
	}

	group_info->m_state = SPU_THREAD_GROUP_STATUS_RUNNING;	//SPU Thread Group now all in running.
	//REMOVE BUSY

	return CELL_OK;
}

s32 sys_spu_thread_group_suspend(u32 id)
{
	sys_spu.Log("sys_spu_thread_group_suspend(id=%d)", id);

	SpuGroupInfo* group_info;
	if(!Emu.GetIdManager().GetIDData(id, group_info))
	{
		return CELL_ESRCH;
	}

	// TODO: check group state
	//Experimental implementation for the state checking
	if ((group_info->m_state != SPU_THREAD_GROUP_STATUS_READY)
		&& (group_info->m_state != SPU_THREAD_GROUP_STATUS_RUNNING)
		&& (group_info->m_state != SPU_THREAD_GROUP_STATUS_WAITING))
	{
		return CELL_ESTAT;
	}

	//Check for BUSY?

	//SET BUSY

	for (u32 i = 0; i < group_info->list.size(); i++)
	{
		if (CPUThread* t = Emu.GetCPU().GetThread(group_info->list[i]))
		{
			t->Pause();
		}
	}

	//Now the state changes.
	if ((group_info->m_state == SPU_THREAD_GROUP_STATUS_READY)
		|| (group_info->m_state == SPU_THREAD_GROUP_STATUS_RUNNING))
	{
		group_info->m_state = SPU_THREAD_GROUP_STATUS_SUSPENDED;
	}
	else if (group_info->m_state == SPU_THREAD_GROUP_STATUS_WAITING)
	{
		group_info->m_state = SPU_THREAD_GROUP_STATUS_WAITING_AND_SUSPENDED;
	}
	//REMOVE BUSY

	return CELL_OK;
}

s32 sys_spu_thread_group_resume(u32 id)
{
	sys_spu.Log("sys_spu_thread_group_resume(id=%d)", id);

	SpuGroupInfo* group_info;
	if(!Emu.GetIdManager().GetIDData(id, group_info))
	{
		return CELL_ESRCH;
	}

	// TODO: check group state
	if ((group_info->m_state != SPU_THREAD_GROUP_STATUS_SUSPENDED)
		&& (group_info->m_state != SPU_THREAD_GROUP_STATUS_WAITING_AND_SUSPENDED))
	{
		return CELL_ESTAT;
	}

	//Maybe check for BUSY

	//SET BUSY

	if (group_info->m_state == SPU_THREAD_GROUP_STATUS_SUSPENDED)
	{
		group_info->m_state = SPU_THREAD_GROUP_STATUS_READY;
	}
	else if (group_info->m_state == SPU_THREAD_GROUP_STATUS_WAITING_AND_SUSPENDED)
	{
		group_info->m_state = SPU_THREAD_GROUP_STATUS_WAITING;
	}

	for (u32 i = 0; i < group_info->list.size(); i++)
	{
		if (CPUThread* t = Emu.GetCPU().GetThread(group_info->list[i]))
		{
			t->Resume();
		}
	}

	if (group_info->m_state == SPU_THREAD_GROUP_STATUS_READY)
	{
		group_info->m_state = SPU_THREAD_GROUP_STATUS_RUNNING;
	}
	//REMOVE BUSY

	return CELL_OK;
}

s32 sys_spu_thread_group_yield(u32 id)
{
	sys_spu.Error("sys_spu_thread_group_yield(id=%d)", id);

	SpuGroupInfo* group_info;
	if (!Emu.GetIdManager().GetIDData(id, group_info))
	{
		return CELL_ESRCH;
	}
	
	////TODO::implement sys_spu_thread_group_yield.
	//Sorry i don't know where to get the caller group. So Only checking.
	//Removed some stupid comments.

	//Check the priority of the target spu group info.
	//And check the state of target spu.
	//if ((group_info->m_prio < current_thread.GetPrio())
	//	||(group_info->m_state != SPU_THREAD_GROUP_STATUS_READY))
	//{
	//	return CELL_OK;
	//}
	
	////Maybe Check for BUSY

	////SET BUSY
	//for (u32 i = 0; i < current_group_info->list.size(); i++)
	//{
		//if (CPUThread* t = Emu.GetCPU().GetThread(current_group_info->list[i]))
		//{
			//Not finding anything that suite the yield test. Do nothing.
			//t->WaitFor(group_info);
		//}
	//}

	//Do nothing now, so not entering the WAITING state.
	//current_group_info->m_state = SPU_THREAD_GROUP_STATUS_WAITING;
	
	////CLEAR BUSY

	return CELL_OK;
}

s32 sys_spu_thread_group_terminate(u32 id, int value)
{
	sys_spu.Error("sys_spu_thread_group_terminate(id=%d, value=%d)", id, value);

	SpuGroupInfo* group_info;
	if (!Emu.GetIdManager().GetIDData(id, group_info))
	{
		return CELL_ESRCH;
	}
	if ((group_info->m_state != SPU_THREAD_GROUP_STATUS_NOT_INITIALIZED)
		&& (group_info->m_state != SPU_THREAD_GROUP_STATUS_INITIALIZED)
		&& (group_info->m_state != SPU_THREAD_GROUP_STATUS_WAITING))
	{
		return CELL_ESTAT;
	}
	//TODO::I don't know who should i be referred to check the EPERM.
	//Also i don't know how to check that is a primary or not. so disabled the EPERM check.
	//Removed some stupid comments made.

	//Attention. This action may not check for BUSY
	
	//SET BUSY
	for (u32 i = 0; i < group_info->list.size(); i++)
	{
		if (CPUThread* t = Emu.GetCPU().GetThread(group_info->list[i]))
		{
			((SPUThread*)t)->SPU.Status.SetValue(SPU_STATUS_STOPPED);
			t->Stop();
		}
	}
	group_info->m_state = SPU_THREAD_GROUP_STATUS_INITIALIZED;	// In initialized state but not running, maybe.
	//Remove BUSY

	group_info->m_exit_status = value;
	
	////TODO::implement sys_spu_thread_group_terminate
	return CELL_OK;
}

SpuGroupInfo* spu_thread_group_create(const std::string& name, u32 num, s32 prio, s32 type, u32 container)
{
	LV2_LOCK(0);

	if (type)
	{
		sys_spu.Todo("Unsupported SPU Thread Group type (0x%x)", type);
	}

	auto group = new SpuGroupInfo(name, num, prio, type, container);
	const u32 _id = sys_spu.GetNewId(group);
	group->m_id = _id;
	sys_spu.Notice("*** SPU Thread Group created [%s] (num=%d, prio=%d, type=0x%x, container=%d): id=%d",
		name.c_str(), num, prio, type, container, _id);
	return group;
}

s32 sys_spu_thread_group_create(vm::ptr<be_t<u32>> id, u32 num, s32 prio, vm::ptr<sys_spu_thread_group_attribute> attr)
{
	sys_spu.Warning("sys_spu_thread_group_create(id_addr=0x%x, num=%d, prio=%d, attr_addr=0x%x)",
		id.addr(), num, prio, attr.addr());

	if (!num || num > 6 || prio < 16 || prio > 255)
	{
		return CELL_EINVAL;
	}

	*id = spu_thread_group_create(std::string(attr->name.get_ptr(), attr->nsize - 1), num, prio, attr->type, attr->ct)->m_id;
	return CELL_OK;
}

s32 sys_spu_thread_group_join(u32 id, vm::ptr<be_t<u32>> cause, vm::ptr<be_t<u32>> status)
{
	sys_spu.Warning("sys_spu_thread_group_join(id=%d, cause_addr=0x%x, status_addr=0x%x)", id, cause.addr(), status.addr());

	SpuGroupInfo* group_info;
	if(!Emu.GetIdManager().GetIDData(id, group_info))
	{
		return CELL_ESRCH;
	}

	if (group_info->lock.exchange(1)) // acquire lock	TODO:: The lock might be replaced.
	{
		return CELL_EBUSY;
	}

	bool all_threads_exit = true;
	for (u32 i = 0; i < group_info->list.size(); i++)
	{
		while (CPUThread* t = Emu.GetCPU().GetThread(group_info->list[i]))
		{
			if (!t->IsRunning())
			{
				if (((SPUThread*)t)->SPU.Status.GetValue() != SPU_STATUS_STOPPED_BY_STOP)
				{
					all_threads_exit = false;
				}
				break;
			}
			if (Emu.IsStopped())
			{
				sys_spu.Warning("sys_spu_thread_group_join(id=%d) aborted", id);
				return CELL_OK;
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
	}

	if (cause)
	{
		*cause = group_info->m_group_exit
			? SYS_SPU_THREAD_GROUP_JOIN_GROUP_EXIT
			: (all_threads_exit
				? SYS_SPU_THREAD_GROUP_JOIN_ALL_THREADS_EXIT
				: SYS_SPU_THREAD_GROUP_JOIN_TERMINATED);
	}

	if (status) *status = group_info->m_exit_status;

	group_info->m_state = SPU_THREAD_GROUP_STATUS_INITIALIZED;
	group_info->lock = 0; // release lock	TODO: this LOCK may be replaced.
	return CELL_OK;
}

s32 sys_spu_thread_create(vm::ptr<be_t<u32>> thread_id, vm::ptr<be_t<u32>> entry, u64 arg, int prio, u32 stacksize, u64 flags, u32 threadname_addr)
{
	sys_spu.Todo("sys_spu_thread_create(thread_id_addr=0x%x, entry_addr=0x%x, arg=0x%llx, prio=%d, stacksize=0x%x, flags=0x%llx, threadname_addr=0x%x",
		thread_id.addr(), entry.addr(), arg, prio, stacksize, flags, threadname_addr);
	return CELL_OK;
}

s32 sys_spu_initialize(u32 max_usable_spu, u32 max_raw_spu)
{
	sys_spu.Warning("sys_spu_initialize(max_usable_spu=%d, max_raw_spu=%d)", max_usable_spu, max_raw_spu);

	if(max_raw_spu > 5)
	{
		return CELL_EINVAL;
	}

	return CELL_OK;
}

s32 sys_spu_thread_write_ls(u32 id, u32 address, u64 value, u32 type)
{
	sys_spu.Log("sys_spu_thread_write_ls(id=%d, address=0x%x, value=0x%llx, type=0x%x)",
		id, address, value, type);

	CPUThread* thr = Emu.GetCPU().GetThread(id);

	if(!thr || thr->GetType() != CPU_THREAD_SPU)
	{
		return CELL_ESRCH;
	}

	if (!thr->IsRunning())
	{
		return CELL_ESTAT;
	}

	if (address >= 0x40000 || address + type > 0x40000 || address % type) // check range and alignment
	{
		return CELL_EINVAL;
	}

	switch (type)
	{
	case 1: (*(SPUThread*)thr).WriteLS8(address, (u8)value); return CELL_OK;
	case 2: (*(SPUThread*)thr).WriteLS16(address, (u16)value); return CELL_OK;
	case 4: (*(SPUThread*)thr).WriteLS32(address, (u32)value); return CELL_OK;
	case 8: (*(SPUThread*)thr).WriteLS64(address, value); return CELL_OK;
	default: return CELL_EINVAL;
	}
}

s32 sys_spu_thread_read_ls(u32 id, u32 address, vm::ptr<be_t<u64>> value, u32 type)
{
	sys_spu.Log("sys_spu_thread_read_ls(id=%d, address=0x%x, value_addr=0x%x, type=0x%x)",
		id, address, value.addr(), type);

	CPUThread* thr = Emu.GetCPU().GetThread(id);

	if(!thr || thr->GetType() != CPU_THREAD_SPU)
	{
		return CELL_ESRCH;
	}

	if (!thr->IsRunning())
	{
		return CELL_ESTAT;
	}

	if (address >= 0x40000 || address + type > 0x40000 || address % type) // check range and alignment
	{
		return CELL_EINVAL;
	}

	switch (type)
	{
	case 1: *value = (*(SPUThread*)thr).ReadLS8(address); return CELL_OK;
	case 2: *value = (*(SPUThread*)thr).ReadLS16(address); return CELL_OK;
	case 4: *value = (*(SPUThread*)thr).ReadLS32(address); return CELL_OK;
	case 8: *value = (*(SPUThread*)thr).ReadLS64(address); return CELL_OK;
	default: return CELL_EINVAL;
	}
}

s32 sys_spu_thread_write_spu_mb(u32 id, u32 value)
{
	sys_spu.Warning("sys_spu_thread_write_spu_mb(id=%d, value=0x%x)", id, value);

	CPUThread* thr = Emu.GetCPU().GetThread(id);

	if(!thr || thr->GetType() != CPU_THREAD_SPU)
	{
		return CELL_ESRCH;
	}

	(*(SPUThread*)thr).SPU.In_MBox.PushUncond(value);

	return CELL_OK;
}

s32 sys_spu_thread_set_spu_cfg(u32 id, u64 value)
{
	sys_spu.Warning("sys_spu_thread_set_spu_cfg(id=%d, value=0x%x)", id, value);

	CPUThread* thr = Emu.GetCPU().GetThread(id);

	if(!thr || thr->GetType() != CPU_THREAD_SPU)
	{
		return CELL_ESRCH;
	}

	if (value > 3)
	{
		return CELL_EINVAL;
	}

	(*(SPUThread*)thr).cfg.value = value;

	return CELL_OK;
}

s32 sys_spu_thread_get_spu_cfg(u32 id, vm::ptr<be_t<u64>> value)
{
	sys_spu.Warning("sys_spu_thread_get_spu_cfg(id=%d, value_addr=0x%x)", id, value.addr());

	CPUThread* thr = Emu.GetCPU().GetThread(id);

	if(!thr || thr->GetType() != CPU_THREAD_SPU)
	{
		return CELL_ESRCH;
	}

	*value = (*(SPUThread*)thr).cfg.value;

	return CELL_OK;
}

s32 sys_spu_thread_write_snr(u32 id, u32 number, u32 value)
{
	sys_spu.Log("sys_spu_thread_write_snr(id=%d, number=%d, value=0x%x)", id, number, value);
	CPUThread* thr = Emu.GetCPU().GetThread(id);

	if(!thr || thr->GetType() != CPU_THREAD_SPU)
	{
		return CELL_ESRCH;
	}

	if (number > 1)
	{
		return CELL_EINVAL;
	}

	(*(SPUThread*)thr).WriteSNR(number ? true : false, value);

	return CELL_OK;
}

s32 sys_spu_thread_group_connect_event(u32 id, u32 eq, u32 et)
{
	sys_spu.Todo("sys_spu_thread_group_connect_event(id=%d, eq=%d, et=0x%x)", id, eq, et);

	return CELL_OK;
}

s32 sys_spu_thread_group_disconnect_event(u32 id, u32 et)
{
	sys_spu.Todo("sys_spu_thread_group_disconnect_event(id=%d, et=0x%x)", id, et);

	return CELL_OK;
}

/*
SPU-Side functions:
s32 sys_spu_thread_receive_event(u32 spuq_num, mem32_t d1, mem32_t d2, mem32_t d3);
s32 sys_spu_thread_send_event(u8 spup, u24 data0, u32 data1);
s32 sys_spu_thread_throw_event(u8 spup, u24 data0, u32 data1);
s32 sys_spu_thread_tryreceive_event(u32 spuq_num, mem32_t d1, mem32_t d2, mem32_t d3);
*/

s32 sys_spu_thread_connect_event(u32 id, u32 eq_id, u32 et, u8 spup)
{
	sys_spu.Warning("sys_spu_thread_connect_event(id=%d, eq_id=%d, event_type=0x%x, spup=%d)", id, eq_id, et, spup);

	CPUThread* thr = Emu.GetCPU().GetThread(id);

	if(!thr || thr->GetType() != CPU_THREAD_SPU)
	{
		return CELL_ESRCH;
	}

	EventQueue* eq;
	if (!Emu.GetIdManager().GetIDData(eq_id, eq))
	{
		return CELL_ESRCH;
	}

	if (spup > 63)
	{
		sys_spu.Error("sys_spu_thread_connect_event: invalid spup (%d)", spup);
		return CELL_EINVAL;
	}

	if (et != SYS_SPU_THREAD_EVENT_USER)
	{
		sys_spu.Error("sys_spu_thread_connect_event: unsupported event type (0x%x)", et);
		return CELL_EINVAL;
	}

	// TODO: check if can receive these events

	SPUThread& spu = *(SPUThread*)thr;

	EventPort& port = spu.SPUPs[spup];

	std::lock_guard<std::mutex> lock(port.m_mutex);

	if (port.eq)
	{
		return CELL_EISCONN;
	}

	eq->ports.add(&port);
	port.eq = eq;

	return CELL_OK;
}

s32 sys_spu_thread_disconnect_event(u32 id, u32 et, u8 spup)
{
	sys_spu.Warning("sys_spu_thread_disconnect_event(id=%d, event_type=0x%x, spup=%d)", id, et, spup);

	CPUThread* thr = Emu.GetCPU().GetThread(id);

	if(!thr || thr->GetType() != CPU_THREAD_SPU)
	{
		return CELL_ESRCH;
	}

	if (spup > 63)
	{
		sys_spu.Error("sys_spu_thread_connect_event: invalid spup (%d)", spup);
		return CELL_EINVAL;
	}

	if (et != SYS_SPU_THREAD_EVENT_USER)
	{
		sys_spu.Error("sys_spu_thread_connect_event: unsupported event type (0x%x)", et);
		return CELL_EINVAL;
	}

	SPUThread& spu = *(SPUThread*)thr;

	EventPort& port = spu.SPUPs[spup];

	std::lock_guard<std::mutex> lock(port.m_mutex);

	if (!port.eq)
	{
		return CELL_ENOTCONN;
	}

	port.eq->ports.remove(&port);
	port.eq = nullptr;

	return CELL_OK;
}

s32 sys_spu_thread_bind_queue(u32 id, u32 eq_id, u32 spuq_num)
{
	sys_spu.Warning("sys_spu_thread_bind_queue(id=%d, equeue_id=%d, spuq_num=0x%x)", id, eq_id, spuq_num);

	EventQueue* eq;
	if (!Emu.GetIdManager().GetIDData(eq_id, eq))
	{
		return CELL_ESRCH;
	}

	if (eq->type != SYS_SPU_QUEUE)
	{
		return CELL_EINVAL;
	}

	CPUThread* thr = Emu.GetCPU().GetThread(id);

	if(!thr || thr->GetType() != CPU_THREAD_SPU)
	{
		return CELL_ESRCH;
	}

	if (!(*(SPUThread*)thr).SPUQs.RegisterKey(eq, FIX_SPUQ(spuq_num)))
	{
		return CELL_EBUSY;
	}

	return CELL_OK;
}

s32 sys_spu_thread_unbind_queue(u32 id, u32 spuq_num)
{
	sys_spu.Warning("sys_spu_thread_unbind_queue(id=0x%x, spuq_num=0x%x)", id, spuq_num);

	CPUThread* thr = Emu.GetCPU().GetThread(id);

	if(!thr || thr->GetType() != CPU_THREAD_SPU)
	{
		return CELL_ESRCH;
	}

	if (!(*(SPUThread*)thr).SPUQs.UnregisterKey(FIX_SPUQ(spuq_num)))
	{
		return CELL_ESRCH; // may be CELL_EINVAL
	}

	return CELL_OK;
}

s32 sys_spu_thread_group_connect_event_all_threads(u32 id, u32 eq_id, u64 req, vm::ptr<u8> spup)
{
	sys_spu.Warning("sys_spu_thread_group_connect_event_all_threads(id=%d, eq_id=%d, req=0x%llx, spup_addr=0x%x)",
		id, eq_id, req, spup.addr());

	EventQueue* eq;
	if (!Emu.GetIdManager().GetIDData(eq_id, eq))
	{
		return CELL_ESRCH;
	}

	if (!req)
	{
		return CELL_EINVAL;
	}

	SpuGroupInfo* group;
	if (!Emu.GetIdManager().GetIDData(id, group))
	{
		return CELL_ESRCH;
	}

	std::vector<SPUThread*> threads;
	for (auto& v : group->list)
	{
		if (!v) continue;
		CPUThread* thr = Emu.GetCPU().GetThread(v);
		if (thr->GetType() != CPU_THREAD_SPU)
		{
			sys_spu.Error("sys_spu_thread_group_connect_event_all_threads(): CELL_ESTAT (wrong thread type)");
			return CELL_ESTAT;
		}
		threads.push_back((SPUThread*)thr);
	}

	if (threads.size() != group->m_count)
	{
		sys_spu.Error("sys_spu_thread_group_connect_event_all_threads(): CELL_ESTAT (%d from %d)", (u32)threads.size(), group->m_count);
		return CELL_ESTAT;
	}

	for (u32 i = 0; i < 64; i++) // port number
	{
		bool found = true;
		if (req & (1ull << i))
		{
			for (auto& t : threads) t->SPUPs[i].m_mutex.lock();

			for (auto& t : threads) if (t->SPUPs[i].eq) found = false;

			if (found)
			{
				for (auto& t : threads)
				{
					eq->ports.add(&(t->SPUPs[i]));
					t->SPUPs[i].eq = eq;
				}
				sys_spu.Warning("*** spup -> %d", i);
				*spup = (u8)i;
			}

			for (auto& t : threads) t->SPUPs[i].m_mutex.unlock();
		}
		else
		{
			found = false;
		}

		if (found) return CELL_OK;
	}

	return CELL_EISCONN;
}

s32 sys_spu_thread_group_disconnect_event_all_threads(u32 id, u8 spup)
{
	sys_spu.Todo("sys_spu_thread_group_disconnect_event_all_threads(id=%d, spup=%d)", id, spup);

	return CELL_OK;
}

s32 sys_raw_spu_create(vm::ptr<be_t<u32>> id, u32 attr_addr)
{
	sys_spu.Warning("sys_raw_spu_create(id_addr=0x%x, attr_addr=0x%x)", id.addr(), attr_addr);

	CPUThread& new_thread = Emu.GetCPU().AddThread(CPU_THREAD_RAW_SPU);
	if (((RawSPUThread&)new_thread).GetIndex() >= 5)
	{
		Emu.GetCPU().RemoveThread(new_thread.GetId());
		return CELL_EAGAIN;
	}

	*id = ((RawSPUThread&)new_thread).GetIndex();
	new_thread.Run();
	return CELL_OK;
}

s32 sys_raw_spu_destroy(u32 id)
{
	sys_spu.Warning("sys_raw_spu_destroy(id=%d)", id);

	RawSPUThread* t = Emu.GetCPU().GetRawSPUThread(id);

	if (!t)
	{
		return CELL_ESRCH;
	}

	// TODO: check if busy

	Emu.GetCPU().RemoveThread(t->GetId());
	return CELL_OK;
}

s32 sys_raw_spu_create_interrupt_tag(u32 id, u32 class_id, u32 hwthread, vm::ptr<be_t<u32>> intrtag)
{
	sys_spu.Warning("sys_raw_spu_create_interrupt_tag(id=%d, class_id=%d, hwthread=0x%x, intrtag_addr=0x%x)", id, class_id, hwthread, intrtag.addr());

	RawSPUThread* t = Emu.GetCPU().GetRawSPUThread(id);

	if (!t)
	{
		return CELL_ESRCH;
	}

	if (class_id != 0 && class_id != 2)
	{
		return CELL_EINVAL;
	}

	if (t->m_intrtag[class_id].enabled)
	{
		return CELL_EAGAIN;
	}

	t->m_intrtag[class_id].enabled = 1;
	*intrtag = (id & 0xff) | (class_id << 8);

	return CELL_OK;
}

s32 sys_raw_spu_set_int_mask(u32 id, u32 class_id, u64 mask)
{
	sys_spu.Warning("sys_raw_spu_set_int_mask(id=%d, class_id=%d, mask=0x%llx)", id, class_id, mask);

	RawSPUThread* t = Emu.GetCPU().GetRawSPUThread(id);
	if (!t)
	{
		return CELL_ESRCH;
	}

	if (class_id != 0 && class_id != 2)
	{
		return CELL_EINVAL;
	}

	t->m_intrtag[class_id].mask = mask; // TODO: check this
	return CELL_OK;
}

s32 sys_raw_spu_get_int_mask(u32 id, u32 class_id, vm::ptr<be_t<u64>> mask)
{
	sys_spu.Log("sys_raw_spu_get_int_mask(id=%d, class_id=%d, mask_addr=0x%x)", id, class_id, mask.addr());

	RawSPUThread* t = Emu.GetCPU().GetRawSPUThread(id);
	if (!t)
	{
		return CELL_ESRCH;
	}

	if (class_id != 0 && class_id != 2)
	{
		return CELL_EINVAL;
	}

	*mask = t->m_intrtag[class_id].mask;
	return CELL_OK;
}

s32 sys_raw_spu_set_int_stat(u32 id, u32 class_id, u64 stat)
{
	sys_spu.Log("sys_raw_spu_set_int_stat(id=%d, class_id=%d, stat=0x%llx)", id, class_id, stat);

	RawSPUThread* t = Emu.GetCPU().GetRawSPUThread(id);
	if (!t)
	{
		return CELL_ESRCH;
	}

	if (class_id != 0 && class_id != 2)
	{
		return CELL_EINVAL;
	}

	t->m_intrtag[class_id].stat = stat; // TODO: check this
	return CELL_OK;
}

s32 sys_raw_spu_get_int_stat(u32 id, u32 class_id, vm::ptr<be_t<u64>> stat)
{
	sys_spu.Log("sys_raw_spu_get_int_stat(id=%d, class_id=%d, stat_addr=0xx)", id, class_id, stat.addr());

	RawSPUThread* t = Emu.GetCPU().GetRawSPUThread(id);
	if (!t)
	{
		return CELL_ESRCH;
	}

	if (class_id != 0 && class_id != 2)
	{
		return CELL_EINVAL;
	}

	*stat = t->m_intrtag[class_id].stat;
	return CELL_OK;
}

s32 sys_raw_spu_read_puint_mb(u32 id, vm::ptr<be_t<u32>> value)
{
	sys_spu.Log("sys_raw_spu_read_puint_mb(id=%d, value_addr=0x%x)", id, value.addr());

	RawSPUThread* t = Emu.GetCPU().GetRawSPUThread(id);
	if (!t)
	{
		return CELL_ESRCH;
	}

	u32 v;
	t->SPU.Out_IntrMBox.PopUncond(v);
	*value = v;
	return CELL_OK;
}

s32 sys_raw_spu_set_spu_cfg(u32 id, u32 value)
{
	sys_spu.Log("sys_raw_spu_set_spu_cfg(id=%d, value=0x%x)", id, value);

	RawSPUThread* t = Emu.GetCPU().GetRawSPUThread(id);
	if (!t)
	{
		return CELL_ESRCH;
	}

	t->cfg.value = value;
	return CELL_OK;
}

s32 sys_raw_spu_get_spu_cfg(u32 id, vm::ptr<be_t<u32>> value)
{
	sys_spu.Log("sys_raw_spu_get_spu_afg(id=%d, value_addr=0x%x)", id, value.addr());

	RawSPUThread* t = Emu.GetCPU().GetRawSPUThread(id);
	if (!t)
	{
		return CELL_ESRCH;
	}

	*value = (u32)t->cfg.value;
	return CELL_OK;
}
