#include "stdafx.h"
#include "Utilities/Log.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/Cell/PPUThread.h"
#include "Emu/FS/vfsFile.h"
#include "sys_spu.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/SC_FUNC.h"
#include "Loader/ELF.h"
#include "Emu/Cell/RawSPUThread.h"
#include <atomic>

static SysCallBase sc_spu("sys_spu");
extern SysCallBase sys_event;

u32 LoadSpuImage(vfsStream& stream, u32& spu_ep)
{
	ELFLoader l(stream);
	l.LoadInfo();
	const u32 alloc_size = 256 * 1024;
	u32 spu_offset = Memory.MainMem.AllocAlign(alloc_size);
	l.LoadData(spu_offset);
	spu_ep = l.GetEntry();
	return spu_offset;
}

//156
s32 sys_spu_image_open(mem_ptr_t<sys_spu_image> img, u32 path_addr)
{
	const std::string path = Memory.ReadString(path_addr);
	sc_spu.Warning("sys_spu_image_open(img_addr=0x%x, path_addr=0x%x [%s])", img.GetAddr(), path_addr, path.c_str());

	vfsFile f(path);
	if(!f.IsOpened())
	{
		sc_spu.Error("sys_spu_image_open error: '%s' not found!", path.c_str());
		return CELL_ENOENT;
	}

	u32 entry;
	u32 offset = LoadSpuImage(f, entry);

	img->type = 1;
	img->entry_point = entry;
	img->segs_addr = offset;
	img->nsegs = 0;

	return CELL_OK;
}

//172
s32 sys_spu_thread_initialize(mem32_t thread, u32 group, u32 spu_num, mem_ptr_t<sys_spu_image> img, mem_ptr_t<sys_spu_thread_attribute> attr, mem_ptr_t<sys_spu_thread_argument> arg)
{
	sc_spu.Warning("sys_spu_thread_initialize(thread_addr=0x%x, group=0x%x, spu_num=%d, img_addr=0x%x, attr_addr=0x%x, arg_addr=0x%x)",
		thread.GetAddr(), group, spu_num, img.GetAddr(), attr.GetAddr(), arg.GetAddr());

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

	u32 spu_ep = (u32)img->entry_point;

	std::string name = "SPUThread";
	if (attr->name_addr)
	{
		name = Memory.ReadString(attr->name_addr, attr->name_len);
	}

	u64 a1 = arg->arg1;
	u64 a2 = arg->arg2;
	u64 a3 = arg->arg3;
	u64 a4 = arg->arg4;

	//copy SPU image:
	auto spu_offset = Memory.MainMem.AllocAlign(256 * 1024);
	memcpy(Memory + spu_offset, Memory + (u32)img->segs_addr, 256 * 1024);

	CPUThread& new_thread = Emu.GetCPU().AddThread(CPU_THREAD_SPU);
	//initialize from new place:
	new_thread.SetOffset(spu_offset);
	new_thread.SetEntry(spu_ep);
	new_thread.SetName(name);
	new_thread.SetArg(0, a1);
	new_thread.SetArg(1, a2);
	new_thread.SetArg(2, a3);
	new_thread.SetArg(3, a4);
	new_thread.Run();

	thread = group_info->list[spu_num] = new_thread.GetId();
	(*(SPUThread*)&new_thread).group = group_info;

	sc_spu.Warning("*** New SPU Thread [%s] (img_offset=0x%x, ls_offset=0x%x, ep=0x%x, a1=0x%llx, a2=0x%llx, a3=0x%llx, a4=0x%llx): id=%d",
		(attr->name_addr ? name.c_str() : ""), (u32) img->segs_addr, ((SPUThread&) new_thread).dmac.ls_offset, spu_ep, a1, a2, a3, a4, thread.GetValue());

	return CELL_OK;
}

//166
s32 sys_spu_thread_set_argument(u32 id, mem_ptr_t<sys_spu_thread_argument> arg)
{
	sc_spu.Warning("sys_spu_thread_set_argument(id=%d, arg_addr=0x%x)", id, arg.GetAddr());
	CPUThread* thr = Emu.GetCPU().GetThread(id);

	if(!thr || thr->GetType() != CPU_THREAD_SPU)
	{
		return CELL_ESRCH;
	}

	thr->SetArg(0, arg->arg1);
	thr->SetArg(1, arg->arg2);
	thr->SetArg(2, arg->arg3);
	thr->SetArg(3, arg->arg4);

	return CELL_OK;
}

//165
s32 sys_spu_thread_get_exit_status(u32 id, mem32_t status)
{
	sc_spu.Warning("sys_spu_thread_get_exit_status(id=%d, status_addr=0x%x)", id, status.GetAddr());

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

	status = res;
	return CELL_OK;
}

//171
s32 sys_spu_thread_group_destroy(u32 id)
{
	sc_spu.Warning("sys_spu_thread_group_destroy(id=%d)", id);

	SpuGroupInfo* group_info;
	if(!Emu.GetIdManager().GetIDData(id, group_info))
	{
		return CELL_ESRCH;
	}

	if (group_info->lock) // ???
	{
		return CELL_EBUSY;
	}

	for (u32 i = 0; i < group_info->list.size(); i++)
	{
		// TODO: disconnect all event ports

		Emu.GetCPU().RemoveThread(group_info->list[i]);
	}

	Emu.GetIdManager().RemoveID(id);
	return CELL_OK;
}

//173
s32 sys_spu_thread_group_start(u32 id)
{
	sc_spu.Warning("sys_spu_thread_group_start(id=%d)", id);

	SpuGroupInfo* group_info;
	if(!Emu.GetIdManager().GetIDData(id, group_info))
	{
		return CELL_ESRCH;
	}

	// TODO: check group state

	for (u32 i = 0; i < group_info->list.size(); i++)
	{
		CPUThread* t = Emu.GetCPU().GetThread(group_info->list[i]);
		if (t)
		{
			t->Exec();
		}
	}

	return CELL_OK;
}

//174
s32 sys_spu_thread_group_suspend(u32 id)
{
	sc_spu.Log("sys_spu_thread_group_suspend(id=%d)", id);

	SpuGroupInfo* group_info;
	if(!Emu.GetIdManager().GetIDData(id, group_info))
	{
		return CELL_ESRCH;
	}

	// TODO: check group state

	for (u32 i = 0; i < group_info->list.size(); i++)
	{
		if (CPUThread* t = Emu.GetCPU().GetThread(group_info->list[i]))
		{
			t->Pause();
		}
	}

	return CELL_OK;
}

//175
s32 sys_spu_thread_group_resume(u32 id)
{
	sc_spu.Log("sys_spu_thread_group_resume(id=%d)", id);

	SpuGroupInfo* group_info;
	if(!Emu.GetIdManager().GetIDData(id, group_info))
	{
		return CELL_ESRCH;
	}

	// TODO: check group state

	for (u32 i = 0; i < group_info->list.size(); i++)
	{
		if (CPUThread* t = Emu.GetCPU().GetThread(group_info->list[i]))
		{
			t->Resume();
		}
	}

	return CELL_OK;
}

//170
s32 sys_spu_thread_group_create(mem32_t id, u32 num, int prio, mem_ptr_t<sys_spu_thread_group_attribute> attr)
{
	sc_spu.Warning("sys_spu_thread_group_create(id_addr=0x%x, num=%d, prio=%d, attr_addr=0x%x)",
		id.GetAddr(), num, prio, attr.GetAddr());

	if (num > 256) return CELL_EINVAL;

	if (prio < 16 || prio > 255) return CELL_EINVAL;

	const std::string name = Memory.ReadString(attr->name_addr, attr->name_len);

	id = sc_spu.GetNewId(new SpuGroupInfo(name, num, prio, attr->type, attr->ct));

	sc_spu.Warning("*** SPU Thread Group created [%s] (type=0x%x, option.ct=0x%x): id=%d", 
		name.c_str(), (int)attr->type, (u32)attr->ct, id.GetValue());

	return CELL_OK;
}

//178
s32 sys_spu_thread_group_join(u32 id, mem32_t cause, mem32_t status)
{
	sc_spu.Warning("sys_spu_thread_group_join(id=%d, cause_addr=0x%x, status_addr=0x%x)", id, cause.GetAddr(), status.GetAddr());

	SpuGroupInfo* group_info;
	if(!Emu.GetIdManager().GetIDData(id, group_info))
	{
		return CELL_ESRCH;
	}

	if (group_info->lock.exchange(1)) // acquire lock
	{
		return CELL_EBUSY;
	}

	if (cause.GetAddr()) cause = SYS_SPU_THREAD_GROUP_JOIN_ALL_THREADS_EXIT;
	if (status.GetAddr()) status = 0; //unspecified because of ALL_THREADS_EXIT

	for (u32 i = 0; i < group_info->list.size(); i++)
	{
		while (CPUThread* t = Emu.GetCPU().GetThread(group_info->list[i]))
		{
			if (!t->IsRunning())
			{
				break;
			}
			if (Emu.IsStopped())
			{
				LOG_WARNING(Log::SPU, "sys_spu_thread_group_join(id=%d) aborted", id);
				return CELL_OK;
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
	}

	group_info->lock = 0; // release lock
	return CELL_OK;
}

s32 sys_spu_thread_create(mem32_t thread_id, mem32_t entry, u64 arg, int prio, u32 stacksize, u64 flags, u32 threadname_addr)
{
	sc_spu.Todo("sys_spu_thread_create(thread_id_addr=0x%x, entry_addr=0x%x, arg=0x%llx, prio=%d, stacksize=0x%x, flags=0x%llx, threadname_addr=0x%x",
		thread_id.GetAddr(), entry.GetAddr(), arg, prio, stacksize, flags, threadname_addr);
	return CELL_OK;
}

//169
s32 sys_spu_initialize(u32 max_usable_spu, u32 max_raw_spu)
{
	sc_spu.Warning("sys_spu_initialize(max_usable_spu=%d, max_raw_spu=%d)", max_usable_spu, max_raw_spu);

	if(max_raw_spu > 5)
	{
		return CELL_EINVAL;
	}

	return CELL_OK;
}

//181
s32 sys_spu_thread_write_ls(u32 id, u32 address, u64 value, u32 type)
{
	sc_spu.Log("sys_spu_thread_write_ls(id=%d, address=0x%x, value=0x%llx, type=0x%x)",
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
	case 1: (*(SPUThread*)thr).WriteLS8(address, value); return CELL_OK;
	case 2: (*(SPUThread*)thr).WriteLS16(address, value); return CELL_OK;
	case 4: (*(SPUThread*)thr).WriteLS32(address, value); return CELL_OK;
	case 8: (*(SPUThread*)thr).WriteLS64(address, value); return CELL_OK;
	default: return CELL_EINVAL;
	}
}

//182
s32 sys_spu_thread_read_ls(u32 id, u32 address, mem64_t value, u32 type)
{
	sc_spu.Log("sys_spu_thread_read_ls(id=%d, address=0x%x, value_addr=0x%x, type=0x%x)",
		id, address, value.GetAddr(), type);

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
	case 1: value = (*(SPUThread*)thr).ReadLS8(address); return CELL_OK;
	case 2: value = (*(SPUThread*)thr).ReadLS16(address); return CELL_OK;
	case 4: value = (*(SPUThread*)thr).ReadLS32(address); return CELL_OK;
	case 8: value = (*(SPUThread*)thr).ReadLS64(address); return CELL_OK;
	default: return CELL_EINVAL;
	}
}

//190
s32 sys_spu_thread_write_spu_mb(u32 id, u32 value)
{
	sc_spu.Log("sys_spu_thread_write_spu_mb(id=%d, value=0x%x)", id, value);

	CPUThread* thr = Emu.GetCPU().GetThread(id);

	if(!thr || thr->GetType() != CPU_THREAD_SPU)
	{
		return CELL_ESRCH;
	}

	(*(SPUThread*)thr).SPU.In_MBox.PushUncond(value);

	return CELL_OK;
}

//187
s32 sys_spu_thread_set_spu_cfg(u32 id, u64 value)
{
	sc_spu.Warning("sys_spu_thread_set_spu_cfg(id=%d, value=0x%x)", id, value);

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

//188
s32 sys_spu_thread_get_spu_cfg(u32 id, mem64_t value)
{
	sc_spu.Warning("sys_spu_thread_get_spu_cfg(id=%d, value_addr=0x%x)", id, value.GetAddr());

	CPUThread* thr = Emu.GetCPU().GetThread(id);

	if(!thr || thr->GetType() != CPU_THREAD_SPU)
	{
		return CELL_ESRCH;
	}

	value = (*(SPUThread*)thr).cfg.value;

	return CELL_OK;
}

//184
s32 sys_spu_thread_write_snr(u32 id, u32 number, u32 value)
{
	sc_spu.Log("sys_spu_thread_write_snr(id=%d, number=%d, value=0x%x)", id, number, value);
	CPUThread* thr = Emu.GetCPU().GetThread(id);

	if(!thr || thr->GetType() != CPU_THREAD_SPU)
	{
		return CELL_ESRCH;
	}

	if (number > 1)
	{
		return CELL_EINVAL;
	}

	(*(SPUThread*)thr).WriteSNR(number, value);

	return CELL_OK;
}

s32 sys_spu_thread_group_connect_event(u32 id, u32 eq, u32 et)
{
	sc_spu.Todo("sys_spu_thread_group_connect_event(id=%d, eq=%d, et=0x%x)", id, eq, et);

	return CELL_OK;
}

s32 sys_spu_thread_group_disconnect_event(u32 id, u32 et)
{
	sc_spu.Todo("sys_spu_thread_group_disconnect_event(id=%d, et=0x%x)", id, et);

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
	sc_spu.Warning("sys_spu_thread_connect_event(id=%d, eq_id=%d, event_type=0x%x, spup=%d)", id, eq_id, et, spup);

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
		sc_spu.Error("sys_spu_thread_connect_event: invalid spup (%d)", spup);
		return CELL_EINVAL;
	}

	if (et != SYS_SPU_THREAD_EVENT_USER)
	{
		sc_spu.Error("sys_spu_thread_connect_event: unsupported event type (0x%x)", et);
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

//
s32 sys_spu_thread_disconnect_event(u32 id, u32 et, u8 spup)
{
	sc_spu.Warning("sys_spu_thread_disconnect_event(id=%d, event_type=0x%x, spup=%d)", id, et, spup);

	CPUThread* thr = Emu.GetCPU().GetThread(id);

	if(!thr || thr->GetType() != CPU_THREAD_SPU)
	{
		return CELL_ESRCH;
	}

	if (spup > 63)
	{
		sc_spu.Error("sys_spu_thread_connect_event: invalid spup (%d)", spup);
		return CELL_EINVAL;
	}

	if (et != SYS_SPU_THREAD_EVENT_USER)
	{
		sc_spu.Error("sys_spu_thread_connect_event: unsupported event type (0x%x)", et);
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
	sc_spu.Warning("sys_spu_thread_bind_queue(id=%d, equeue_id=%d, spuq_num=0x%x)", id, eq_id, spuq_num);

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
	sc_spu.Warning("sys_spu_thread_unbind_queue(id=0x%x, spuq_num=0x%x)", id, spuq_num);

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

s32 sys_spu_thread_group_connect_event_all_threads(u32 id, u32 eq_id, u64 req, mem8_t spup)
{
	sc_spu.Warning("sys_spu_thread_group_connect_event_all_threads(id=%d, eq_id=%d, req=0x%llx, spup_addr=0x%x)",
		id, eq_id, req, spup.GetAddr());

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
			sc_spu.Error("sys_spu_thread_group_connect_event_all_threads(): CELL_ESTAT (wrong thread type)");
			return CELL_ESTAT;
		}
		threads.push_back((SPUThread*)thr);
	}

	if (threads.size() != group->m_count)
	{
		sc_spu.Error("sys_spu_thread_group_connect_event_all_threads(): CELL_ESTAT (%d from %d)", (u32)threads.size(), group->m_count);
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
				spup = (u8)i;
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
	sc_spu.Todo("sys_spu_thread_group_disconnect_event_all_threads(id=%d, spup=%d)", id, spup);

	return CELL_OK;
}

//160
s32 sys_raw_spu_create(mem32_t id, u32 attr_addr)
{
	sc_spu.Warning("sys_raw_spu_create(id_addr=0x%x, attr_addr=0x%x)", id.GetAddr(), attr_addr);

	CPUThread& new_thread = Emu.GetCPU().AddThread(CPU_THREAD_RAW_SPU);
	if (((RawSPUThread&)new_thread).GetIndex() >= 5)
	{
		Emu.GetCPU().RemoveThread(new_thread.GetId());
		return CELL_EAGAIN;
	}

	id = ((RawSPUThread&)new_thread).GetIndex();
	new_thread.Run();
	return CELL_OK;
}

s32 sys_raw_spu_destroy(u32 id)
{
	sc_spu.Warning("sys_raw_spu_destroy(id=%d)", id);

	RawSPUThread* t = Emu.GetCPU().GetRawSPUThread(id);

	if (!t)
	{
		return CELL_ESRCH;
	}

	// TODO: check if busy

	Emu.GetCPU().RemoveThread(t->GetId());
	return CELL_OK;
}

s32 sys_raw_spu_create_interrupt_tag(u32 id, u32 class_id, u32 hwthread, mem32_t intrtag)
{
	sc_spu.Warning("sys_raw_spu_create_interrupt_tag(id=%d, class_id=%d, hwthread=0x%x, intrtag_addr=0x%x)", id, class_id, hwthread, intrtag.GetAddr());

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
	intrtag = (id & 0xff) | (class_id << 8);

	return CELL_OK;
}

s32 sys_raw_spu_set_int_mask(u32 id, u32 class_id, u64 mask)
{
	sc_spu.Warning("sys_raw_spu_set_int_mask(id=%d, class_id=%d, mask=0x%llx)", id, class_id, mask);

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

s32 sys_raw_spu_get_int_mask(u32 id, u32 class_id, mem64_t mask)
{
	sc_spu.Log("sys_raw_spu_get_int_mask(id=%d, class_id=%d, mask_addr=0x%x)", id, class_id, mask.GetAddr());

	RawSPUThread* t = Emu.GetCPU().GetRawSPUThread(id);
	if (!t)
	{
		return CELL_ESRCH;
	}

	if (class_id != 0 && class_id != 2)
	{
		return CELL_EINVAL;
	}

	mask = t->m_intrtag[class_id].mask;
	return CELL_OK;
}

s32 sys_raw_spu_set_int_stat(u32 id, u32 class_id, u64 stat)
{
	sc_spu.Log("sys_raw_spu_set_int_stat(id=%d, class_id=%d, stat=0x%llx)", id, class_id, stat);

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

s32 sys_raw_spu_get_int_stat(u32 id, u32 class_id, mem64_t stat)
{
	sc_spu.Log("sys_raw_spu_get_int_stat(id=%d, class_id=%d, stat_addr=0xx)", id, class_id, stat.GetAddr());

	RawSPUThread* t = Emu.GetCPU().GetRawSPUThread(id);
	if (!t)
	{
		return CELL_ESRCH;
	}

	if (class_id != 0 && class_id != 2)
	{
		return CELL_EINVAL;
	}

	stat = t->m_intrtag[class_id].stat;
	return CELL_OK;
}

s32 sys_raw_spu_read_puint_mb(u32 id, mem32_t value)
{
	sc_spu.Log("sys_raw_spu_read_puint_mb(id=%d, value_addr=0x%x)", id, value.GetAddr());

	RawSPUThread* t = Emu.GetCPU().GetRawSPUThread(id);
	if (!t)
	{
		return CELL_ESRCH;
	}

	u32 v;
	t->SPU.Out_IntrMBox.PopUncond(v);
	value = v;
	return CELL_OK;
}

s32 sys_raw_spu_set_spu_cfg(u32 id, u32 value)
{
	sc_spu.Log("sys_raw_spu_set_spu_cfg(id=%d, value=0x%x)", id, value);

	RawSPUThread* t = Emu.GetCPU().GetRawSPUThread(id);
	if (!t)
	{
		return CELL_ESRCH;
	}

	t->cfg.value = value;
	return CELL_OK;
}

s32 sys_raw_spu_get_spu_cfg(u32 id, mem32_t value)
{
	sc_spu.Log("sys_raw_spu_get_spu_afg(id=%d, value_addr=0x%x)", id, value.GetAddr());

	RawSPUThread* t = Emu.GetCPU().GetRawSPUThread(id);
	if (!t)
	{
		return CELL_ESRCH;
	}

	value = t->cfg.value;
	return CELL_OK;
}
