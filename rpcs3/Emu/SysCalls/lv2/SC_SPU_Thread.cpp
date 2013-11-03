#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Loader/ELF.h"
#include "Emu/Cell/RawSPUThread.h"

SysCallBase sc_spu("sys_spu");

struct sys_spu_thread_group_attribute
{
	u32 name_len;
	u32 name_addr;
	int type;
	union{u32 ct;} option;
};

struct sys_spu_thread_attribute
{
	u32 name_addr;
	u32 name_len;
	u32 option;
};

struct sys_spu_thread_argument
{
	u64 arg1;
	u64 arg2;
	u64 arg3;
	u64 arg4;
};

struct sys_spu_image
{
	u32 type;
	u32 entry_point;
	u32 segs_addr;
	int nsegs;
};

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

u32 LoadSpuImage(vfsStream& stream)
{
	ELFLoader l(stream);
	l.LoadInfo();
	g_spu_offset = Memory.MainMem.Alloc(0xFFFFED - stream.GetSize());
	l.LoadData(g_spu_offset);

	return g_spu_offset + l.GetEntry();
}

//156
int sys_spu_image_open(u32 img_addr, u32 path_addr)
{
	const wxString& path = Memory.ReadString(path_addr);
	sc_spu.Warning("sys_spu_image_open(img_addr=0x%x, path_addr=0x%x [%s])", img_addr, path_addr, path);

	if(!Memory.IsGoodAddr(img_addr, sizeof(sys_spu_image)) || !Memory.IsGoodAddr(path_addr))
	{
		return CELL_EFAULT;
	}

	vfsStream* stream = Emu.GetVFS().Open(path, vfsRead);

	if(!stream || !stream->IsOpened())
	{
		sc_spu.Error("sys_spu_image_open error: '%s' not found!", path);
		delete stream;

		return CELL_ENOENT;
	}

	u32 entry = LoadSpuImage(*stream);
	delete stream;

	auto& ret = (sys_spu_image&)Memory[img_addr];
	re(ret.type, 1);
	re(ret.entry_point, entry);
	re(ret.segs_addr, 0x0);
	re(ret.nsegs, 0);

	return CELL_OK;
}

//172
int sys_spu_thread_initialize(u32 thread_addr, u32 group, u32 spu_num, u32 img_addr, u32 attr_addr, u32 arg_addr)
{
	sc_spu.Warning("sys_spu_thread_initialize(thread_addr=0x%x, group=0x%x, spu_num=%d, img_addr=0x%x, attr_addr=0x%x, arg_addr=0x%x)",
		thread_addr, group, spu_num, img_addr, attr_addr, arg_addr);

	if(!Emu.GetIdManager().CheckID(group))
	{
		return CELL_ESRCH;
	}

	SpuGroupInfo& group_info = *(SpuGroupInfo*)Emu.GetIdManager().GetIDData(group).m_data;

	if(
		!Memory.IsGoodAddr(img_addr, sizeof(sys_spu_image)) ||
		!Memory.IsGoodAddr(attr_addr, sizeof(sys_spu_thread_attribute)) ||
		!Memory.IsGoodAddr(arg_addr, sizeof(sys_spu_thread_argument)))
	{
		return CELL_EFAULT;
	}

	auto& img = (sys_spu_image&)Memory[img_addr];
	auto& attr = (sys_spu_thread_attribute&)Memory[attr_addr];
	auto& arg = (sys_spu_thread_argument&)Memory[arg_addr];

	if(!Memory.IsGoodAddr(re(attr.name_addr), re(attr.name_len)))
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

	u32 entry = re(img.entry_point);
	wxString name = Memory.ReadString(re(attr.name_addr), re(attr.name_len));
	u64 a1 = re(arg.arg1);
	u64 a2 = re(arg.arg2);
	u64 a3 = re(arg.arg3);
	u64 a4 = re(arg.arg4);

	ConLog.Write("New SPU Thread:");
	ConLog.Write("entry = 0x%x", entry);
	ConLog.Write("name = %s", name);
	ConLog.Write("a1 = 0x%x", a1);
	ConLog.Write("a2 = 0x%x", a2);
	ConLog.Write("a3 = 0x%x", a3);
	ConLog.Write("a4 = 0x%x", a4);
	ConLog.SkipLn();

	CPUThread& new_thread = Emu.GetCPU().AddThread(CPU_THREAD_SPU);
	new_thread.SetOffset(g_spu_offset);
	new_thread.SetEntry(entry - g_spu_offset);
	new_thread.SetName(name);
	new_thread.Run();
	new_thread.Pause();
	new_thread.SetArg(0, a1);
	new_thread.SetArg(1, a2);
	new_thread.SetArg(2, a3);
	new_thread.SetArg(3, a4);

	group_info.threads[spu_num] = &new_thread;

	return CELL_OK;
}

//166
int sys_spu_thread_set_argument(u32 id, u32 arg_addr)
{
	sc_spu.Warning("sys_spu_thread_set_argument(id=0x%x, arg_addr=0x%x)", id, arg_addr);
	CPUThread* thr = Emu.GetCPU().GetThread(id);

	if(!thr || (thr->GetType() != CPU_THREAD_SPU && thr->GetType() != CPU_THREAD_RAW_SPU))
	{
		return CELL_ESRCH;
	}

	if(!Memory.IsGoodAddr(arg_addr, sizeof(sys_spu_thread_argument)))
	{
		return CELL_EFAULT;
	}

	auto& arg = (sys_spu_thread_argument&)Memory[arg_addr];
	thr->SetArg(0, re(arg.arg1));
	thr->SetArg(1, re(arg.arg2));
	thr->SetArg(2, re(arg.arg3));
	thr->SetArg(3, re(arg.arg4));

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

	Emu.Pause();
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
int sys_spu_thread_group_create(u64 id_addr, u32 num, int prio, u64 attr_addr)
{
	ConLog.Write("sys_spu_thread_group_create:");
	ConLog.Write("*** id_addr=0x%llx", id_addr);
	ConLog.Write("*** num=%d", num);
	ConLog.Write("*** prio=%d", prio);
	ConLog.Write("*** attr_addr=0x%llx", attr_addr);

	sys_spu_thread_group_attribute& attr = (sys_spu_thread_group_attribute&)Memory[attr_addr];

	ConLog.Write("*** attr.name_len=%d", re(attr.name_len));
	ConLog.Write("*** attr.name_addr=0x%x", re(attr.name_addr));
	ConLog.Write("*** attr.type=%d", re(attr.type));
	ConLog.Write("*** attr.option.ct=%d", re(attr.option.ct));

	const wxString& name = Memory.ReadString(re(attr.name_addr), re(attr.name_len));
	ConLog.Write("*** name='%s'", name);

	Memory.Write32(id_addr,
		Emu.GetIdManager().GetNewID(wxString::Format("sys_spu_thread_group '%s'", name), new SpuGroupInfo(attr)));

	return CELL_OK;
}

int sys_spu_thread_create(u64 thread_id_addr, u64 entry_addr, u64 arg,
	int prio, u32 stacksize, u64 flags, u64 threadname_addr)
{
	return CELL_OK;
}

int sys_spu_thread_connect_event(u32 id, u32 eq, u32 et, u8 spup)
{
	if(!Emu.GetIdManager().CheckID(id))
	{
		return CELL_ESRCH;
	}

	if(spup > 63)
	{
		return CELL_EINVAL;
	}

	return CELL_OK;
}

//160
int sys_raw_spu_create(u32 id_addr, u32 attr_addr)
{
	sc_spu.Warning("sys_raw_spu_create(id_addr=0x%x, attr_addr=0x%x)", id_addr, attr_addr);

	//Emu.GetIdManager().GetNewID("sys_raw_spu", new u32(attr_addr));
	CPUThread& new_thread = Emu.GetCPU().AddThread(CPU_THREAD_RAW_SPU);
	Memory.Write32(id_addr, ((RawSPUThread&)new_thread).GetIndex());
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
int sys_spu_thread_read_ls(u32 id, u32 address, u32 value_addr, u32 type)
{
	sc_spu.Warning("sys_spu_thread_read_ls(id=0x%x, address=0x%x, value_addr=0x%x, type=0x%x)",
		id, address, value_addr, type);

	CPUThread* thr = Emu.GetCPU().GetThread(id);

	if(!thr || (thr->GetType() != CPU_THREAD_SPU && thr->GetType() != CPU_THREAD_RAW_SPU))
	{
		return CELL_ESRCH;
	}

	if(!(*(SPUThread*)thr).IsGoodLSA(address))
	{
		return CELL_EFAULT;
	}

	Memory.Write64(value_addr, (*(SPUThread*)thr).ReadLS64(address));

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

extern SysCallBase sys_event;

int sys_spu_thread_group_connect_event_all_threads(u32 id, u32 eq, u64 req, u32 spup_addr)
{
	sc_spu.Warning("sys_spu_thread_group_connect_event_all_threads(id=0x%x, eq=0x%x, req=0x%llx, spup_addr=0x%x)",
		id, eq, req, spup_addr);

	if(!Emu.GetIdManager().CheckID(id) || !sys_event.CheckId(eq))
	{
		return CELL_ESRCH;
	}

	if(!req)
	{
		return CELL_EINVAL;
	}

	SpuGroupInfo* group = (SpuGroupInfo*)Emu.GetIdManager().GetIDData(id).m_data;
	EventQueue* equeue = (EventQueue*)Emu.GetIdManager().GetIDData(eq).m_data;
	
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