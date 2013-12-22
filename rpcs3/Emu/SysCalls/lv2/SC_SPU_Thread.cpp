#include "stdafx.h"
#include "SC_SPU_Thread.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Loader/ELF.h"
#include "Emu/Cell/RawSPUThread.h"

static SysCallBase sc_spu("sys_spu");
extern SysCallBase sys_event;

static const u32 g_spu_group_thr_count = 255;

struct SpuGroupInfo
{
	CPUThread* threads[g_spu_group_thr_count];
	sys_spu_thread_group_attribute& attr;

	SpuGroupInfo(sys_spu_thread_group_attribute& attr) : attr(attr)
	{
		memset(threads, 0, sizeof(CPUThread*) * g_spu_group_thr_count);
	}
};

u64 g_spu_offset = 0;
u64 g_spu_alloc_size = 0;

u32 LoadSpuImage(vfsStream& stream)
{
	ELFLoader l(stream);
	l.LoadInfo();
	g_spu_alloc_size = 0xFFFFED - stream.GetSize();
	g_spu_offset = Memory.MainMem.Alloc(g_spu_alloc_size);
	l.LoadData(g_spu_offset);

	return g_spu_offset + l.GetEntry();
}

//156
int sys_spu_image_open(mem_ptr_t<sys_spu_image> img, u32 path_addr)
{
	const std::string& path = Memory.ReadString(path_addr).mb_str();
	sc_spu.Warning("sys_spu_image_open(img_addr=0x%x, path_addr=0x%x [%s])", img.GetAddr(), path_addr, path);

	if(!img.IsGood() || !Memory.IsGoodAddr(path_addr))
	{
		return CELL_EFAULT;
	}

	vfsFile f(path.c_str());
	if(!f.IsOpened())
	{
		sc_spu.Error("sys_spu_image_open error: '%s' not found!", path);
		return CELL_ENOENT;
	}

	u32 entry = LoadSpuImage(f);

	img->type = 1;
	img->entry_point = entry;
	img->segs_addr = 0x0;
	img->nsegs = 0;

	return CELL_OK;
}

//172
int sys_spu_thread_initialize(mem32_t thread, u32 group, u32 spu_num, mem_ptr_t<sys_spu_image> img, mem_ptr_t<sys_spu_thread_attribute> attr, mem_ptr_t<sys_spu_thread_argument> arg)
{
	sc_spu.Warning("sys_spu_thread_initialize(thread_addr=0x%x, group=0x%x, spu_num=%d, img_addr=0x%x, attr_addr=0x%x, arg_addr=0x%x)",
		thread.GetAddr(), group, spu_num, img.GetAddr(), attr.GetAddr(), arg.GetAddr());

	if(!Emu.GetIdManager().CheckID(group))
	{
		return CELL_ESRCH;
	}

	SpuGroupInfo& group_info = *(SpuGroupInfo*)Emu.GetIdManager().GetIDData(group).m_data;

	if(!thread.IsGood() || !img.IsGood() || !attr.IsGood() || !attr.IsGood())
	{
		return CELL_EFAULT;
	}

	if(!Memory.IsGoodAddr(attr->name_addr, attr->name_len))
	{
		return CELL_EFAULT;
	}

	if(spu_num >= g_spu_group_thr_count)
	{
		return CELL_EINVAL;
	}
	
	if(group_info.threads[spu_num])
	{
		return CELL_EBUSY;
	}

	u32 ls_entry = img->entry_point - g_spu_offset;
	std::string name = Memory.ReadString(attr->name_addr, attr->name_len).mb_str();
	u64 a1 = arg->arg1;
	u64 a2 = arg->arg2;
	u64 a3 = arg->arg3;
	u64 a4 = arg->arg4;

	CPUThread& new_thread = Emu.GetCPU().AddThread(CPU_THREAD_SPU);
	//copy SPU image:
	u32 spu_offset = Memory.MainMem.Alloc(256 * 1024);
	memcpy(Memory + spu_offset, Memory + g_spu_offset, 256 * 1024);
	//initialize from new place:
	new_thread.SetOffset(spu_offset);
	new_thread.SetEntry(ls_entry);
	new_thread.SetName(name);
	new_thread.SetArg(0, a1);
	new_thread.SetArg(1, a2);
	new_thread.SetArg(2, a3);
	new_thread.SetArg(3, a4);
	new_thread.Run();

	thread = new_thread.GetId();

	group_info.threads[spu_num] = &new_thread;

	ConLog.Write("New SPU Thread:");
	ConLog.Write("ls_entry = 0x%x", ls_entry);
	ConLog.Write("name = %s", name);
	ConLog.Write("a1 = 0x%x", a1);
	ConLog.Write("a2 = 0x%x", a2);
	ConLog.Write("a3 = 0x%x", a3);
	ConLog.Write("a4 = 0x%x", a4);
	ConLog.Write("ls_offset = 0x%x", ((SPUThread&)new_thread).dmac.ls_offset);
	ConLog.SkipLn();

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

//173
int sys_spu_thread_group_start(u32 id)
{
	sc_spu.Warning("sys_spu_thread_group_start(id=0x%x)", id);

	if(!Emu.GetIdManager().CheckID(id))
	{
		return CELL_ESRCH;
	}

	ID& id_data = Emu.GetIdManager().GetIDData(id);
	SpuGroupInfo& group_info = *(SpuGroupInfo*)id_data.m_data;

	//Emu.Pause();
	for(int i=0; i<g_spu_group_thr_count; i++)
	{
		if(group_info.threads[i])
		{
			group_info.threads[i]->Exec();
		}
	}

	return CELL_OK;
}

//170
int sys_spu_thread_group_create(mem32_t id, u32 num, int prio, mem_ptr_t<sys_spu_thread_group_attribute> attr)
{
	ConLog.Write("sys_spu_thread_group_create:");
	ConLog.Write("*** id_addr=0x%x", id.GetAddr());
	ConLog.Write("*** num=%d", num);
	ConLog.Write("*** prio=%d", prio);
	ConLog.Write("*** attr_addr=0x%x", attr.GetAddr());

	ConLog.Write("*** attr.name_len=%d", attr->name_len.ToLE());
	ConLog.Write("*** attr.name_addr=0x%x", attr->name_addr.ToLE());
	ConLog.Write("*** attr.type=%d", attr->type.ToLE());
	ConLog.Write("*** attr.option.ct=%d", attr->option.ct.ToLE());

	const std::string& name = Memory.ReadString(attr->name_addr, attr->name_len).mb_str();
	ConLog.Write("*** name='%s'", name);

	id = Emu.GetIdManager().GetNewID(wxString::Format("sys_spu_thread_group '%s'", name), new SpuGroupInfo(*attr));

	return CELL_OK;
}

int sys_spu_thread_create(mem32_t thread_id, mem32_t entry, u64 arg, int prio, u32 stacksize, u64 flags, u32 threadname_addr)
{
	UNIMPLEMENTED_FUNC(sc_spu);
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
	CPUThread* thr = Emu.GetCPU().GetThread(id);

	if(!thr || (thr->GetType() != CPU_THREAD_SPU && thr->GetType() != CPU_THREAD_RAW_SPU))
	{
		return CELL_ESRCH;
	}

	if (number > 1)
	{
		return CELL_EINVAL;
	}

	if ((*(SPUThread*)thr).cfg.value & (1<<number))
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
	if(!Emu.GetIdManager().CheckID(id) || !sys_event.CheckId(eq, equeue))
	{
		return CELL_ESRCH;
	}

	if(!req)
	{
		return CELL_EINVAL;
	}

	SpuGroupInfo* group = (SpuGroupInfo*)Emu.GetIdManager().GetIDData(id).m_data;
	
	for(int i=0; i<g_spu_group_thr_count; ++i)
	{
		if(group->threads[i])
		{
			bool finded_port = false;
			for(int j=0; j<equeue->pos; ++j)
			{
				if(!equeue->ports[j]->thread)
				{
					finded_port = true;
					equeue->ports[j]->thread = group->threads[i];
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
