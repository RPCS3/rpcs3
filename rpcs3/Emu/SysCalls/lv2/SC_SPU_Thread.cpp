#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"

SysCallBase sc_spu("sys_spu");

struct sys_spu_thread_group_attribute
{
	u32 name_len;
	u32 name_addr;
	int type;
	union{u32 ct;} option;
};

//170
int sys_spu_thread_group_create(u64 id_addr, u32 num, int prio, u64 attr_addr)
{
	ConLog.Write("sys_spu_thread_group_create:");
	ConLog.Write("*** id_addr=0x%llx", id_addr);
	ConLog.Write("*** num=%d", num);
	ConLog.Write("*** prio=%d", prio);
	ConLog.Write("*** attr_addr=0x%llx", attr_addr);

	sys_spu_thread_group_attribute& attr = *new sys_spu_thread_group_attribute(*(sys_spu_thread_group_attribute*)&Memory[attr_addr]);

	ConLog.Write("*** attr.name_len=%d", re(attr.name_len));
	ConLog.Write("*** attr.name_addr=0x%x", re(attr.name_addr));
	ConLog.Write("*** attr.type=%d", re(attr.type));
	ConLog.Write("*** attr.option.ct=%d", re(attr.option.ct));

	const wxString& name = Memory.ReadString(re(attr.name_addr), re(attr.name_len));
	ConLog.Write("*** name='%s'", name);

	Memory.Write32(id_addr,
		Emu.GetIdManager().GetNewID(wxString::Format("sys_spu_thread_group %s", name), &attr, 0));

	return CELL_OK;
}

int sys_spu_thread_create(u64 thread_id_addr, u64 entry_addr, u64 arg,
	int prio, u32 stacksize, u64 flags, u64 threadname_addr)
{
	return CELL_OK;
}

//160
int sys_raw_spu_create(u32 id_addr, u32 attr_addr)
{
	sc_spu.Log("sys_raw_spu_create(id_addr=0x%x, attr_addr=0x%x)", id_addr, attr_addr);

	PPCThread& new_thread = Emu.GetCPU().AddThread(false);
	Emu.GetIdManager().GetNewID("sys_raw_spu", new u32(attr_addr));
	Memory.Write32(id_addr, Emu.GetCPU().GetThreadNumById(false, new_thread.GetId()));

	return CELL_OK;
}

//169
int sys_spu_initialize(u32 max_usable_spu, u32 max_raw_spu)
{
	sc_spu.Log("sys_spu_initialize(max_usable_spu=%d, max_raw_spu=%d)", max_usable_spu, max_raw_spu);

	if(!Memory.InitSpuRawMem(max_raw_spu))
	{
		return CELL_UNKNOWN_ERROR;
	}

	return CELL_OK;
}