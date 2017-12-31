#pragma once

#include "Emu/Cell/PPUAnalyser.h"
#include "sys_sync.h"

//Unknown error codes

struct ppu_overlay_module final : lv2_obj, ppu_module
{
	static const u32 id_base = 0x24000000;	//arbitrary base
};

struct sys_overlay_t {
	be_t<u32> id;	//additional data unknown
};


error_code sys_overlay_load_module(vm::ps3::ptr<sys_overlay_t> ovlmid, vm::ps3::cptr<char> path, uint64_t flags, vm::ps3::ptr<u32> entry);
error_code sys_overlay_unload_module(u32 ovlmid);
//error_code sys_overlay_get_module_list(sys_pid_t pid, size_t ovlmids_num, sys_overlay_t * ovlmids, size_t * num_of_modules);
//error_code sys_overlay_get_module_info(sys_pid_t pid, sys_overlay_t ovlmid, sys_overlay_module_info_t * info);
//error_code sys_overlay_load_module_by_fd(sys_overlay_t * ovlmid, int fd, u64 offset, uint64_t flags, sys_addr_t * entry);
//error_code sys_overlay_get_module_info2(sys_pid_t pid, sys_overlay_t ovlmid, sys_overlay_module_info2_t * info);//
//error_code sys_overlay_get_sdk_version(); //2 params
//error_code sys_overlay_get_module_dbg_info(); //3 params?

//error_code _sys_prx_load_module(vm::ps3::cptr<char> path, u64 flags, vm::ps3::ptr<sys_prx_load_module_option_t> pOpt);
