#pragma once

#include "Emu/Cell/PPUAnalyser.h"
#include "Emu/Memory/vm_ptr.h"
#include "sys_sync.h"

struct lv2_overlay final : ppu_module<lv2_obj>
{
	static const u32 id_base = 0x25000000;

	u32 entry{};
	u32 seg0_code_end{};

	lv2_overlay() = default;
	lv2_overlay(utils::serial&){}
	static std::function<void(void*)> load(utils::serial& ar);
	void save(utils::serial& ar);
};

error_code sys_overlay_load_module(vm::ptr<u32> ovlmid, vm::cptr<char> path, u64 flags, vm::ptr<u32> entry);
error_code sys_overlay_load_module_by_fd(vm::ptr<u32> ovlmid, u32 fd, u64 offset, u64 flags, vm::ptr<u32> entry);
error_code sys_overlay_unload_module(u32 ovlmid);
//error_code sys_overlay_get_module_list(sys_pid_t pid, usz ovlmids_num, sys_overlay_t * ovlmids, usz * num_of_modules);
//error_code sys_overlay_get_module_info(sys_pid_t pid, sys_overlay_t ovlmid, sys_overlay_module_info_t * info);
//error_code sys_overlay_get_module_info2(sys_pid_t pid, sys_overlay_t ovlmid, sys_overlay_module_info2_t * info);//
//error_code sys_overlay_get_sdk_version(); //2 params
//error_code sys_overlay_get_module_dbg_info(); //3 params?

//error_code _sys_prx_load_module(vm::ps3::cptr<char> path, u64 flags, vm::ps3::ptr<sys_prx_load_module_option_t> pOpt);
