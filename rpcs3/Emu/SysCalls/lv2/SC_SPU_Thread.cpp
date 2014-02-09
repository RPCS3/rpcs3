#include "stdafx.h"
#include "SC_SPU_Thread.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/SC_FUNC.h"
#include "Loader/ELF.h"
#include "Emu/Cell/RawSPUThread.h"
#include <atomic>

static SysCallBase sc_spu("sys_spu");
extern SysCallBase sys_event;

static const u32 g_spu_group_thr_max = 255;

struct SpuGroupInfo
{
	Array<u32> list;
	std::atomic<u32> lock;
	wxString m_name;
	int m_prio;
	int m_type;
	int m_ct;

	SpuGroupInfo(wxString name, u32 num, int prio, int type, u32 ct) 
		: m_name(name)
		, m_prio(prio)
		, m_type(type)
		, m_ct(ct)
		, lock(0)
	{
		list.SetCount(num);
		for (u32 i = 0; i < num; i++)
		{
			list[i] = 0;
		}
	}
};

u32 LoadSpuImage(vfsStream& stream, u32& spu_ep)
{
	ELFLoader l(stream);
	l.LoadInfo();
	const u32 alloc_size = 256 * 1024 /*0x1000000 - stream.GetSize()*/;
	u32 spu_offset = Memory.MainMem.AllocAlign(alloc_size);
	l.LoadData(spu_offset);
	spu_ep = l.GetEntry();
	return spu_offset;
}
/*u64 g_last_spu_offset = 0;
static const u64 g_spu_alloc_size = 0x1000000;

u32 LoadSpuImage(vfsStream& stream, u64 address)
{
	ELFLoader l(stream);
	l.LoadInfo();
	l.LoadData(address);

	return address + l.GetEntry();
}

u32 LoadSpuImage(vfsStream& stream)
{
	g_last_spu_offset = Memory.MainMem.Alloc(g_spu_alloc_size);
	return LoadSpuImage(stream, g_last_spu_offset);
}*/

//156
int sys_spu_image_open(mem_ptr_t<sys_spu_image> img, u32 path_addr)
{
	const wxString path = Memory.ReadString(path_addr).mb_str();
	sc_spu.Warning("sys_spu_image_open(img_addr=0x%x, path_addr=0x%x [%s])", img.GetAddr(), path_addr, path.c_str());

	if(!img.IsGood() || !Memory.IsGoodAddr(path_addr))
	{
		return CELL_EFAULT;
	}

	vfsFile f(path);
	if(!f.IsOpened())
	{
		sc_spu.Error("sys_spu_image_open error: '%s' not found!", path);
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
int sys_spu_thread_initialize(mem32_t thread, u32 group, u32 spu_num, mem_ptr_t<sys_spu_image> img, mem_ptr_t<sys_spu_thread_attribute> attr, mem_ptr_t<sys_spu_thread_argument> arg)
{
	sc_spu.Warning("sys_spu_thread_initialize(thread_addr=0x%x, group=0x%x, spu_num=%d, img_addr=0x%x, attr_addr=0x%x, arg_addr=0x%x)",
		thread.GetAddr(), group, spu_num, img.GetAddr(), attr.GetAddr(), arg.GetAddr());

	SpuGroupInfo* group_info;
	if(!Emu.GetIdManager().GetIDData(group, group_info))
	{
		return CELL_ESRCH;
	}

	if(!thread.IsGood() || !img.IsGood() || !attr.IsGood() || !arg.IsGood())
	{
		return CELL_EFAULT;
	}

	if(!Memory.IsGoodAddr(attr->name_addr, attr->name_len))
	{
		return CELL_EFAULT;
	}

	if(spu_num >= g_spu_group_thr_max)
	{
		return CELL_EINVAL;
	}
	
	if(group_info->list[spu_num])
	{
		return CELL_EBUSY;
	}

	u32 spu_ep = (u32)img->entry_point;
	std::string name = Memory.ReadString(attr->name_addr, attr->name_len).mb_str();
	u64 a1 = arg->arg1;
	u64 a2 = arg->arg2;
	u64 a3 = arg->arg3;
	u64 a4 = arg->arg4;

	CPUThread& new_thread = Emu.GetCPU().AddThread(CPU_THREAD_SPU);
	//copy SPU image:
	u32 spu_offset = Memory.MainMem.AllocAlign(256 * 1024);
	memcpy(Memory + spu_offset, Memory + (u32)img->segs_addr, 256 * 1024);
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

	sc_spu.Warning("*** New SPU Thread [%s] (img_offset=0x%x, ls_offset=0x%x, ep=0x%x, a1=0x%llx, a2=0x%llx, a3=0x%llx, a4=0x%llx): id=%d",
		name.c_str(), (u32)img->segs_addr, ((SPUThread&)new_thread).dmac.ls_offset, spu_ep, a1, a2, a3, a4, thread.GetValue());

	return CELL_OK;
}

//166
int sys_spu_thread_set_argument(u32 id, mem_ptr_t<sys_spu_thread_argument> arg)
{
	sc_spu.Warning("sys_spu_thread_set_argument(id=0x%x, arg_addr=0x%x)", id, arg.GetAddr());
	CPUThread* thr = Emu.GetCPU().GetThread(id);

	if(!thr || (thr->GetType() != CPU_THREAD_SPU && thr->GetType() != CPU_THREAD_RAW_SPU))
	{
		return CELL_ESRCH;
	}

	if(!arg.IsGood())
	{
		return CELL_EFAULT;
	}

	thr->SetArg(0, arg->arg1);
	thr->SetArg(1, arg->arg2);
	thr->SetArg(2, arg->arg3);
	thr->SetArg(3, arg->arg4);

	return CELL_OK;
}

//165
int sys_spu_thread_get_exit_status(u32 id, mem32_t status)
{
	sc_spu.Warning("sys_spu_thread_get_exit_status(id=0x%x, status_addr=0x%x)", id, status.GetAddr());

	if (!status.IsGood())
	{
		return CELL_EFAULT;
	}

	CPUThread* thr = Emu.GetCPU().GetThread(id);

	if(!thr || (thr->GetType() != CPU_THREAD_SPU && thr->GetType() != CPU_THREAD_RAW_SPU))
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
int sys_spu_thread_group_destroy(u32 id)
{
	sc_spu.Warning("sys_spu_thread_group_destroy(id=0x%x)", id);

	SpuGroupInfo* group_info;
	if(!Emu.GetIdManager().GetIDData(id, group_info))
	{
		return CELL_ESRCH;
	}

	if (group_info->lock) // ???
	{
		return CELL_EBUSY;
	}

	for (u32 i = 0; i < group_info->list.GetCount(); i++)
	{
		Emu.GetCPU().RemoveThread(group_info->list[i]);
	}

	Emu.GetIdManager().RemoveID(id);
	return CELL_OK;
}

//173
int sys_spu_thread_group_start(u32 id)
{
	sc_spu.Warning("sys_spu_thread_group_start(id=0x%x)", id);

	SpuGroupInfo* group_info;
	if(!Emu.GetIdManager().GetIDData(id, group_info))
	{
		return CELL_ESRCH;
	}

	for (u32 i = 0; i < group_info->list.GetCount(); i++)
	{
		CPUThread* t;
		if (t = Emu.GetCPU().GetThread(group_info->list[i]))
		{
			t->Exec();
		}
	}

	return CELL_OK;
}

//174
int sys_spu_thread_group_suspend(u32 id)
{
	sc_spu.Warning("sys_spu_thread_group_suspend(id=0x%x)", id);

	SpuGroupInfo* group_info;
	if(!Emu.GetIdManager().GetIDData(id, group_info))
	{
		return CELL_ESRCH;
	}

	//Emu.Pause();
	for (u32 i = 0; i < group_info->list.GetCount(); i++)
	{
		if (CPUThread* t = Emu.GetCPU().GetThread(group_info->list[i]))
		{
			t->Pause();
		}
	}

	return CELL_OK;
}

//170
int sys_spu_thread_group_create(mem32_t id, u32 num, int prio, mem_ptr_t<sys_spu_thread_group_attribute> attr)
{
	sc_spu.Warning("sys_spu_thread_group_create(id_addr=0x%x, num=%d, prio=%d, attr_addr=0x%x)",
		id.GetAddr(), num, prio, attr.GetAddr());

	if (!id.IsGood() || !attr.IsGood()) return CELL_EFAULT;

	if (!Memory.IsGoodAddr(attr->name_addr, attr->name_len)) return CELL_EFAULT;

	if (num > g_spu_group_thr_max) return CELL_EINVAL;

	if (prio < 16 || prio > 255) return CELL_EINVAL;

	const wxString name = Memory.ReadString(attr->name_addr, attr->name_len);

	id = sc_spu.GetNewId(new SpuGroupInfo(name, num, prio, attr->type, attr->ct));

	sc_spu.Warning("*** SPU Thread Group created [%s] (type=%d, option.ct=%d): id=%d", 
		name.c_str(), (int)attr->type, (u32)attr->ct, id.GetValue());

	return CELL_OK;
}

//178
int sys_spu_thread_group_join(u32 id, mem32_t cause, mem32_t status)
{
	sc_spu.Warning("sys_spu_thread_group_join(id=0x%x, cause_addr=0x%x, status_addr=0x%x)", id, cause.GetAddr(), status.GetAddr());

	SpuGroupInfo* group_info;
	if(!Emu.GetIdManager().GetIDData(id, group_info))
	{
		return CELL_ESRCH;
	}

	if (group_info->lock.exchange(1)) // acquire lock
	{
		return CELL_EBUSY;
	}

	cause = SYS_SPU_THREAD_GROUP_JOIN_ALL_THREADS_EXIT;
	status = 0; //unspecified because of ALL_THREADS_EXIT

	for (u32 i = 0; i < group_info->list.GetCount(); i++)
	{
		while (CPUThread* t = Emu.GetCPU().GetThread(group_info->list[i]))
		{
			if (!t->IsRunning())
			{
				break;
			}
			if (Emu.IsStopped()) return CELL_OK;
			Sleep(1);
		}
	}

	group_info->lock = 0; // release lock
	return CELL_OK;
}

int sys_spu_thread_create(mem32_t thread_id, mem32_t entry, u64 arg, int prio, u32 stacksize, u64 flags, u32 threadname_addr)
{
	sc_spu.Error("sys_spu_thread_create(thread_id_addr=0x%x, entry_addr=0x%x, arg=0x%llx, prio=%d, stacksize=0x%x, flags=0x%llx, threadname_addr=0x%x",
		thread_id.GetAddr(), entry.GetAddr(), arg, prio, stacksize, flags, threadname_addr);
	return CELL_OK;
}

int sys_spu_thread_connect_event(u32 id, u32 eq, u32 et, u8 spup)
{
	sc_spu.Warning("sys_spu_thread_connect_event(id=0x%x,eq=0x%x,et=0x%x,spup=0x%x)", id, eq, et, spup);

	EventQueue* equeue;
	if(!sys_event.CheckId(eq, equeue))
	{
		return CELL_ESRCH;
	}

	if(spup > 63)
	{
		return CELL_EINVAL;
	}

	CPUThread* thr = Emu.GetCPU().GetThread(id);

	if(!thr || (thr->GetType() != CPU_THREAD_SPU && thr->GetType() != CPU_THREAD_RAW_SPU))
	{
		return CELL_ESRCH;
	}

	for(int j=0; j<equeue->pos; ++j)
	{
		if(!equeue->ports[j]->thread)
		{
			equeue->ports[j]->thread = thr;
			return CELL_OK;
		}
	}

	return CELL_EISCONN;
}

//160
int sys_raw_spu_create(mem32_t id, u32 attr_addr)
{
	sc_spu.Warning("sys_raw_spu_create(id_addr=0x%x, attr_addr=0x%x)", id.GetAddr(), attr_addr);

	//Emu.GetIdManager().GetNewID("sys_raw_spu", new u32(attr_addr));
	CPUThread& new_thread = Emu.GetCPU().AddThread(CPU_THREAD_RAW_SPU);
	id = ((RawSPUThread&)new_thread).GetIndex();
	new_thread.Run();
	new_thread.Exec();

	return CELL_OK;
}

//169
int sys_spu_initialize(u32 max_usable_spu, u32 max_raw_spu)
{
	sc_spu.Warning("sys_spu_initialize(max_usable_spu=%d, max_raw_spu=%d)", max_usable_spu, max_raw_spu);

	if(max_raw_spu > 5)
	{
		return CELL_EINVAL;
	}

	//if(!Memory.InitSpuRawMem(max_raw_spu))
	//{
	//	return CELL_ENOMEM;
	//}

	//enable_log = true;
	//dump_enable = true;

	return CELL_OK;
}

//181
int sys_spu_thread_write_ls(u32 id, u32 address, u64 value, u32 type)
{
	sc_spu.Warning("sys_spu_thread_write_ls(id=0x%x, address=0x%x, value=0x%llx, type=0x%x)",
		id, address, value, type);

	CPUThread* thr = Emu.GetCPU().GetThread(id);

	if(!thr || (thr->GetType() != CPU_THREAD_SPU && thr->GetType() != CPU_THREAD_RAW_SPU))
	{
		return CELL_ESRCH;
	}

	(*(SPUThread*)thr).WriteLS64(address, value);

	return CELL_OK;
}

//182
int sys_spu_thread_read_ls(u32 id, u32 address, mem64_t value, u32 type)
{
	sc_spu.Warning("sys_spu_thread_read_ls(id=0x%x, address=0x%x, value_addr=0x%x, type=0x%x)",
		id, address, value.GetAddr(), type);

	CPUThread* thr = Emu.GetCPU().GetThread(id);

	if(!thr || (thr->GetType() != CPU_THREAD_SPU && thr->GetType() != CPU_THREAD_RAW_SPU))
	{
		return CELL_ESRCH;
	}

	if(!value.IsGood() || !(*(SPUThread*)thr).IsGoodLSA(address))
	{
		return CELL_EFAULT;
	}

	value = (*(SPUThread*)thr).ReadLS64(address);

	return CELL_OK;
}

//190
int sys_spu_thread_write_spu_mb(u32 id, u32 value)
{
	sc_spu.Warning("sys_spu_thread_write_spu_mb(id=0x%x, value=0x%x)", id, value);

	CPUThread* thr = Emu.GetCPU().GetThread(id);

	if(!thr || (thr->GetType() != CPU_THREAD_SPU && thr->GetType() != CPU_THREAD_RAW_SPU))
	{
		return CELL_ESRCH;
	}

	if(!(*(SPUThread*)thr).SPU.In_MBox.Push(value))
	{
		ConLog.Warning("sys_spu_thread_write_spu_mb(id=0x%x, value=0x%x): used all mbox items.");
		return CELL_EBUSY; //?
	}

	return CELL_OK;
}

//187
int sys_spu_thread_set_spu_cfg(u32 id, u64 value)
{
	sc_spu.Warning("sys_spu_thread_set_spu_cfg(id=0x%x, value=0x%x)", id, value);

	CPUThread* thr = Emu.GetCPU().GetThread(id);

	if(!thr || (thr->GetType() != CPU_THREAD_SPU && thr->GetType() != CPU_THREAD_RAW_SPU))
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
int sys_spu_thread_get_spu_cfg(u32 id, mem64_t value)
{
	sc_spu.Warning("sys_spu_thread_get_spu_cfg(id=0x%x, value_addr=0x%x)", id, value.GetAddr());

	CPUThread* thr = Emu.GetCPU().GetThread(id);

	if(!thr || (thr->GetType() != CPU_THREAD_SPU && thr->GetType() != CPU_THREAD_RAW_SPU))
	{
		return CELL_ESRCH;
	}

	value = (*(SPUThread*)thr).cfg.value;

	return CELL_OK;
}

//184
int sys_spu_thread_write_snr(u32 id, u32 number, u32 value)
{
	sc_spu.Log("sys_spu_thread_write_snr(id=0x%x, number=%d, value=0x%x)", id, number, value);
	CPUThread* thr = Emu.GetCPU().GetThread(id);

	if(!thr || (thr->GetType() != CPU_THREAD_SPU && thr->GetType() != CPU_THREAD_RAW_SPU))
	{
		return CELL_ESRCH;
	}

	if (number > 1)
	{
		return CELL_EINVAL;
	}

	if ((*(SPUThread*)thr).cfg.value & ((u64)1<<number))
	{ //logical OR
		(*(SPUThread*)thr).SPU.SNR[number].PushUncond_OR(value);
	}
	else
	{ //overwrite
		(*(SPUThread*)thr).SPU.SNR[number].PushUncond(value);
	}

	return CELL_OK;
}

int sys_spu_thread_group_connect_event_all_threads(u32 id, u32 eq, u64 req, u32 spup_addr)
{
	sc_spu.Warning("sys_spu_thread_group_connect_event_all_threads(id=0x%x, eq=0x%x, req=0x%llx, spup_addr=0x%x)",
		id, eq, req, spup_addr);

	EventQueue* equeue;
	if(!sys_event.CheckId(eq, equeue))
	{
		return CELL_ESRCH;
	}

	if(!req)
	{
		return CELL_EINVAL;
	}

	SpuGroupInfo* group;
	if(!Emu.GetIdManager().GetIDData(id, group))
	{
		return CELL_ESRCH;
	}
	
	for(u32 i=0; i<group->list.GetCount(); ++i)
	{
		CPUThread* t;
		if(t = Emu.GetCPU().GetThread(group->list[i]))
		{
			bool finded_port = false;
			for(int j=0; j<equeue->pos; ++j)
			{
				if(!equeue->ports[j]->thread)
				{
					finded_port = true;
					equeue->ports[j]->thread = t;
				}
			}

			if(!finded_port)
			{
				return CELL_EISCONN;
			}
		}
	}
	return CELL_OK;
}

int sys_spu_thread_bind_queue(u32 id, u32 spuq, u32 spuq_num)
{
	sc_spu.Error("sys_spu_thread_bind_queue(id=0x%x, spuq=0x%x, spuq_num=0x%x)", id, spuq, spuq_num);

	return CELL_OK;
}
