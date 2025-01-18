#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"

#include "Emu/Cell/lv2/sys_lwmutex.h"
#include "Emu/Cell/lv2/sys_prx.h"
#include "sysPrxForUser.h"

LOG_CHANNEL(sysPrxForUser);

extern vm::gvar<sys_lwmutex_t> g_ppu_prx_lwm;

// Convert the array of 32-bit pointers to 64-bit pointers using stack allocation
static auto convert_path_list(vm::cpptr<char> path_list, s32 count)
{
	return vm::var<vm::cptr<char, u64>[]>(count, path_list.get_ptr());
}

// Execute start or stop module function
static void entryx(ppu_thread& ppu, vm::ptr<sys_prx_start_stop_module_option_t> opt, u32 args, vm::ptr<void> argp, vm::ptr<s32> res)
{
	if (opt->entry2.addr() != umax)
	{
		*res = opt->entry2(ppu, opt->entry, args, argp);
		return;
	}

	if (opt->entry.addr() != umax)
	{
		*res = opt->entry(ppu, args, argp);
		return;
	}

	*res = 0;
}

error_code sys_prx_load_module(ppu_thread& ppu, vm::cptr<char> path, u64 flags, vm::ptr<sys_prx_load_module_option_t> pOpt)
{
	sysPrxForUser.warning("sys_prx_load_module(path=%s, flags=0x%x, pOpt=*0x%x)", path, flags, pOpt);

	sys_lwmutex_locker lock(ppu, g_ppu_prx_lwm);

	return _sys_prx_load_module(ppu, path, flags, pOpt);
}

error_code sys_prx_load_module_by_fd(ppu_thread& ppu, s32 fd, u64 offset, u64 flags, vm::ptr<sys_prx_load_module_option_t> pOpt)
{
	sysPrxForUser.warning("sys_prx_load_module_by_fd(fd=%d, offset=0x%x, flags=0x%x, pOpt=*0x%x)", fd, offset, flags, pOpt);

	sys_lwmutex_locker lock(ppu, g_ppu_prx_lwm);

	return _sys_prx_load_module_by_fd(ppu, fd, offset, flags, pOpt);
}

error_code sys_prx_load_module_on_memcontainer(ppu_thread& ppu, vm::cptr<char> path, u32 mem_ct, u64 flags, vm::ptr<sys_prx_load_module_option_t> pOpt)
{
	sysPrxForUser.warning("sys_prx_load_module_on_memcontainer(path=%s, mem_ct=0x%x, flags=0x%x, pOpt=*0x%x)", path, mem_ct, flags, pOpt);

	sys_lwmutex_locker lock(ppu, g_ppu_prx_lwm);

	return _sys_prx_load_module_on_memcontainer(ppu, path, mem_ct, flags, pOpt);
}

error_code sys_prx_load_module_on_memcontainer_by_fd(ppu_thread& ppu, s32 fd, u64 offset, u32 mem_ct, u64 flags, vm::ptr<sys_prx_load_module_option_t> pOpt)
{
	sysPrxForUser.warning("sys_prx_load_module_on_memcontainer_by_fd(fd=%d, offset=0x%x, mem_ct=0x%x, flags=0x%x, pOpt=*0x%x)", fd, offset, mem_ct, flags, pOpt);

	sys_lwmutex_locker lock(ppu, g_ppu_prx_lwm);

	return _sys_prx_load_module_on_memcontainer_by_fd(ppu, fd, offset, mem_ct, flags, pOpt);
}

error_code sys_prx_load_module_list(ppu_thread& ppu, s32 count, vm::cpptr<char> path_list, u64 flags, vm::ptr<sys_prx_load_module_option_t> pOpt, vm::ptr<u32> id_list)
{
	sysPrxForUser.todo("sys_prx_load_module_list(count=%d, path_list=**0x%x, flags=0x%x, pOpt=*0x%x, id_list=*0x%x)", count, path_list, flags, pOpt, id_list);

	sys_lwmutex_locker lock(ppu, g_ppu_prx_lwm);

	return _sys_prx_load_module_list(ppu, count, convert_path_list(path_list, count), flags, pOpt, id_list);
}

error_code sys_prx_load_module_list_on_memcontainer(ppu_thread& ppu, s32 count, vm::cpptr<char> path_list, u32 mem_ct, u64 flags, vm::ptr<sys_prx_load_module_option_t> pOpt, vm::ptr<u32> id_list)
{
	sysPrxForUser.todo("sys_prx_load_module_list_on_memcontainer(count=%d, path_list=**0x%x, mem_ct=0x%x, flags=0x%x, pOpt=*0x%x, id_list=*0x%x)", count, path_list, mem_ct, flags, pOpt, id_list);

	sys_lwmutex_locker lock(ppu, g_ppu_prx_lwm);

	return _sys_prx_load_module_list_on_memcontainer(ppu, count, convert_path_list(path_list, count), mem_ct, flags, pOpt, id_list);
}

error_code sys_prx_start_module(ppu_thread& ppu, u32 id, u32 args, vm::ptr<void> argp, vm::ptr<s32> result, u64 flags, vm::ptr<void> pOpt)
{
	sysPrxForUser.warning("sys_prx_start_module(id=0x%x, args=%u, argp=*0x%x, result=*0x%x, flags=0x%x, pOpt=*0x%x)", id, args, argp, result, flags, pOpt);

	if (!result)
	{
		return CELL_EINVAL;
	}

	sys_lwmutex_locker lock(ppu, g_ppu_prx_lwm);

	vm::var<sys_prx_start_stop_module_option_t> opt;
	opt->size = opt.size();
	opt->cmd  = 1;
	opt->entry2.set(-1);

	const error_code res = _sys_prx_start_module(ppu, id, flags, opt);

	if (res < 0)
	{
		return res;
	}

	entryx(ppu, opt, args, argp, result);

	opt->cmd = 2;
	opt->res = *result;

	_sys_prx_start_module(ppu, id, flags, opt);

	return CELL_OK;
}

error_code sys_prx_stop_module(ppu_thread& ppu, u32 id, u32 args, vm::ptr<void> argp, vm::ptr<s32> result, u64 flags, vm::ptr<void> pOpt)
{
	sysPrxForUser.warning("sys_prx_stop_module(id=0x%x, args=%u, argp=*0x%x, result=*0x%x, flags=0x%x, pOpt=*0x%x)", id, args, argp, result, flags, pOpt);

	if (!result)
	{
		return CELL_EINVAL;
	}

	sys_lwmutex_locker lock(ppu, g_ppu_prx_lwm);

	vm::var<sys_prx_start_stop_module_option_t> opt;
	opt->size = opt.size();
	opt->cmd  = 1;
	opt->entry2.set(-1);

	const error_code res = _sys_prx_stop_module(ppu, id, flags, opt);

	if (res < 0)
	{
		return res;
	}

	entryx(ppu, opt, args, argp, result);

	opt->cmd = 2;
	opt->res = *result;

	_sys_prx_stop_module(ppu, id, flags, opt);

	return CELL_OK;
}

error_code sys_prx_unload_module(ppu_thread& ppu, u32 id, u64 flags, vm::ptr<sys_prx_unload_module_option_t> pOpt)
{
	sysPrxForUser.warning("sys_prx_unload_module(id=0x%x, flags=0x%x, pOpt=*0x%x)", id, flags, pOpt);

	sys_lwmutex_locker lock(ppu, g_ppu_prx_lwm);

	return _sys_prx_unload_module(ppu, id, flags, pOpt);
}

error_code sys_prx_register_library(ppu_thread& ppu, vm::ptr<void> lib_entry)
{
	sysPrxForUser.warning("sys_prx_register_library(lib_entry=*0x%x)", lib_entry);

	sys_lwmutex_locker lock(ppu, g_ppu_prx_lwm);

	return _sys_prx_register_library(ppu, lib_entry);
}

error_code sys_prx_unregister_library(ppu_thread& ppu, vm::ptr<void> lib_entry)
{
	sysPrxForUser.warning("sys_prx_unregister_library(lib_entry=*0x%x)", lib_entry);

	sys_lwmutex_locker lock(ppu, g_ppu_prx_lwm);

	return _sys_prx_unregister_library(ppu, lib_entry);
}

error_code sys_prx_get_module_list(ppu_thread& ppu, u64 flags, vm::ptr<sys_prx_get_module_list_t> info)
{
	sysPrxForUser.trace("sys_prx_get_module_list(flags=0x%x, info=*0x%x)", flags, info);

	if (!info || !info->idlist)
	{
		return CELL_EINVAL;
	}

	// Initialize params
	vm::var<sys_prx_get_module_list_option_t> opt;
	opt->size   = opt.size();
	opt->max    = info->max;
	opt->count  = 0;
	opt->idlist = info->idlist;
	opt->unk    = 0;

	// Call the syscall
	const s32 res = _sys_prx_get_module_list(ppu, 2, opt);

	info->max   = opt->max;
	info->count = opt->count;

	return not_an_error(res);
}

error_code sys_prx_get_module_info(ppu_thread& ppu, u32 id, u64 flags, vm::ptr<sys_prx_module_info_t> info)
{
	sysPrxForUser.trace("sys_prx_get_module_info(id=0x%x, flags=0x%x, info=*0x%x)", id, flags, info);

	if (!info)
	{
		return CELL_EINVAL;
	}

	// Initialize params
	vm::var<sys_prx_module_info_option_t> opt;
	opt->size = opt.size();
	opt->info = info;

	// Call the syscall
	return _sys_prx_get_module_info(ppu, id, flags, opt);
}

error_code sys_prx_get_module_id_by_name(ppu_thread& ppu, vm::cptr<char> name, u64 flags, vm::ptr<sys_prx_get_module_id_by_name_option_t> pOpt)
{
	sysPrxForUser.trace("sys_prx_get_module_id_by_name(name=%s, flags=0x%x, pOpt=*0x%x)", name, flags, pOpt);

	if (flags || pOpt)
	{
		return CELL_EINVAL;
	}

	// Call the syscall
	return _sys_prx_get_module_id_by_name(ppu, name, u64{0}, vm::null);
}

error_code sys_prx_get_module_id_by_address(ppu_thread& ppu, u32 addr)
{
	sysPrxForUser.trace("sys_prx_get_module_id_by_address()");

	// Call the syscall
	return _sys_prx_get_module_id_by_address(ppu, addr);
}

error_code sys_prx_exitspawn_with_level()
{
	sysPrxForUser.todo("sys_prx_exitspawn_with_level()");
	return CELL_OK;
}

error_code sys_prx_get_my_module_id(ppu_thread& ppu_do_not_call, ppu_thread&, ppu_thread&, ppu_thread&) // Do not call directly
{
	sysPrxForUser.trace("sys_prx_get_my_module_id()");

	// Call the syscall using the LR
	return _sys_prx_get_module_id_by_address(ppu_do_not_call, static_cast<u32>(ppu_do_not_call.lr));
}

void sysPrxForUser_sys_prx_init()
{
	REG_FUNC(sysPrxForUser, sys_prx_load_module);
	REG_FUNC(sysPrxForUser, sys_prx_load_module_by_fd);
	REG_FUNC(sysPrxForUser, sys_prx_load_module_on_memcontainer);
	REG_FUNC(sysPrxForUser, sys_prx_load_module_on_memcontainer_by_fd);
	REG_FUNC(sysPrxForUser, sys_prx_load_module_list);
	REG_FUNC(sysPrxForUser, sys_prx_load_module_list_on_memcontainer);
	REG_FUNC(sysPrxForUser, sys_prx_start_module);
	REG_FUNC(sysPrxForUser, sys_prx_stop_module);
	REG_FUNC(sysPrxForUser, sys_prx_unload_module);
	REG_FUNC(sysPrxForUser, sys_prx_register_library);
	REG_FUNC(sysPrxForUser, sys_prx_unregister_library);
	REG_FUNC(sysPrxForUser, sys_prx_get_module_list);
	REG_FUNC(sysPrxForUser, sys_prx_get_module_info);
	REG_FUNC(sysPrxForUser, sys_prx_get_module_id_by_name);
	REG_FUNC(sysPrxForUser, sys_prx_get_module_id_by_address);
	REG_FUNC(sysPrxForUser, sys_prx_exitspawn_with_level);
	REG_FUNC(sysPrxForUser, sys_prx_get_my_module_id);
}
