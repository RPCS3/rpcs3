#include "stdafx.h"
#include "Utilities/Log.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/SysCalls/SysCalls.h"
#include "SC_PRX.h"

SysCallBase sys_prx("sys_prx");

s32 sys_prx_load_module(u32 path_addr, u64 flags, mem_ptr_t<sys_prx_load_module_option_t> pOpt)
{
	if (!Memory.IsGoodAddr(path_addr))
		return CELL_PRX_ERROR_INVAL;

	std::string path = Memory.ReadString(path_addr);
	sys_prx.Error("TODO: sys_prx_load_module(path=\"%s\", flags=0x%llx, pOpt=0x%x)", path.c_str(), flags, pOpt.GetAddr());

	vfsFile f(path);
	if (!f.IsOpened()) {
		return CELL_PRX_ERROR_UNKNOWN_MODULE;
	}

	// Load the PRX into memory
	u64 prx_size = f.GetSize();
	u32 prx_address = Memory.Alloc(prx_size, 4);
	f.Read(Memory.VirtualToRealAddr(prx_address), prx_size);

	// Create the PRX object and return its id
	sys_prx_t* prx = new sys_prx_t(prx_size, prx_address);
	u32 id = sys_prx.GetNewId(prx);
	return id;
}

s32 sys_prx_load_module_on_memcontainer()
{
	sys_prx.Error("TODO: sys_prx_load_module_on_memcontainer()");
	return CELL_OK;
}

s32 sys_prx_load_module_by_fd()
{
	sys_prx.Error("TODO: sys_prx_load_module_by_fd()");
	return CELL_OK;
}

s32 sys_prx_load_module_on_memcontainer_by_fd()
{
	sys_prx.Error("TODO: sys_prx_load_module_on_memcontainer_by_fd()");
	return CELL_OK;
}

s32 sys_prx_start_module(s32 id, u32 args, u32 argp_addr, mem32_t modres, u64 flags, mem_ptr_t<sys_prx_start_module_option_t> pOpt)
{
	sys_prx.Error("TODO: sys_prx_start_module(id=%d, args=%d, argp_addr=0x%x, modres_addr=0x%x, flags=0x%llx, pOpt=0x%x)",
		id, args, argp_addr, modres.GetAddr(), flags, pOpt.GetAddr());

	if (!modres.IsGood())
		return CELL_EINVAL;

	sys_prx_t* prx;
	if (!Emu.GetIdManager().GetIDData(id, prx))
		return CELL_ESRCH;

	if (prx->isStarted)
		return CELL_PRX_ERROR_ALREADY_STARTED;

	return CELL_OK;
}

s32 sys_prx_stop_module(s32 id, u32 args, u32 argp_addr, mem32_t modres, u64 flags, mem_ptr_t<sys_prx_stop_module_option_t> pOpt)
{
	sys_prx.Error("TODO: sys_prx_stop_module(id=%d, args=%d, argp_addr=0x%x, modres_addr=0x%x, flags=0x%llx, pOpt=0x%x)",
		id, args, argp_addr, modres.GetAddr(), flags, pOpt.GetAddr());

	if (!modres.IsGood())
		return CELL_EINVAL;

	sys_prx_t* prx;
	if (!Emu.GetIdManager().GetIDData(id, prx))
		return CELL_ESRCH;

	if (!prx->isStarted)
		return CELL_PRX_ERROR_ALREADY_STOPPED;

	return CELL_OK;
}

s32 sys_prx_unload_module(s32 id, u64 flags, mem_ptr_t<sys_prx_unload_module_option_t> pOpt)
{
	sys_prx.Error("TODO: sys_prx_unload_module(id=%d, flags=0x%llx, pOpt=0x%x)", id, flags, pOpt.GetAddr());

	// Get the PRX, free the used memory and delete the object and its ID
	sys_prx_t* prx;
	if (!Emu.GetIdManager().GetIDData(id, prx))
		return CELL_ESRCH;
	Memory.Free(prx->address);
	Emu.GetIdManager().RemoveID(id);
	
	return CELL_OK;
}

s32 sys_prx_get_module_list()
{
	sys_prx.Error("TODO: sys_prx_get_module_list()");
	return CELL_OK;
}

s32 sys_prx_get_my_module_id()
{
	sys_prx.Error("TODO: sys_prx_get_my_module_id()");
	return CELL_OK;
}

s32 sys_prx_get_module_id_by_address()
{
	sys_prx.Error("TODO: sys_prx_get_module_id_by_address()");
	return CELL_OK;
}

s32 sys_prx_get_module_id_by_name()
{
	sys_prx.Error("TODO: sys_prx_get_module_id_by_name()");
	return CELL_OK;
}

s32 sys_prx_get_module_info()
{
	sys_prx.Error("TODO: sys_prx_get_module_info()");
	return CELL_OK;
}

s32 sys_prx_register_library(u32 lib_addr)
{
	sys_prx.Error("TODO: sys_prx_register_library(lib_addr=0x%x)", lib_addr);
	return CELL_OK;
}

s32 sys_prx_unregister_library()
{
	sys_prx.Error("TODO: sys_prx_unregister_library()");
	return CELL_OK;
}

s32 sys_prx_get_ppu_guid()
{
	sys_prx.Error("TODO: sys_prx_get_ppu_guid()");
	return CELL_OK;
}

s32 sys_prx_register_module()
{
	sys_prx.Error("TODO: sys_prx_register_module()");
	return CELL_OK;
}

s32 sys_prx_query_module()
{
	sys_prx.Error("TODO: sys_prx_query_module()");
	return CELL_OK;
}

s32 sys_prx_link_library()
{
	sys_prx.Error("TODO: sys_prx_link_library()");
	return CELL_OK;
}

s32 sys_prx_unlink_library()
{
	sys_prx.Error("TODO: sys_prx_unlink_library()");
	return CELL_OK;
}

s32 sys_prx_query_library()
{
	sys_prx.Error("TODO: sys_prx_query_library()");
	return CELL_OK;
}

s32 sys_prx_start()
{
	sys_prx.Error("TODO: sys_prx_start()");
	return CELL_OK;
}

s32 sys_prx_stop()
{
	sys_prx.Error("TODO: sys_prx_stop()");
	return CELL_OK;
}
