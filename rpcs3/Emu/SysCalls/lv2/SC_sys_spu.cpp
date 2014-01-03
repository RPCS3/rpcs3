#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"

static SysCallBase sc_spu("sys_spu");

u32 _max_usable_spu = 0;
u32 _max_raw_spu = 0;

int sys_spu_initialize(u32 max_usable_spu, u32 max_raw_spu)
{
	sc_spu.Log("sys_spu_initialize(max_usable_spu=%d, max_raw_spu=%d)", max_usable_spu, max_raw_spu);
	_max_usable_spu = max_usable_spu;
	_max_raw_spu = max_raw_spu;
	return CELL_OK;
}

int sys_raw_spu_create(u32 id_addr, u32 attr_addr)
{
	Memory.Write32(id_addr, Emu.GetIdManager().GetNewID("raw_spu"));
	return CELL_OK;
}
