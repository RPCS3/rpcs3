#include "stdafx.h"
#include "sys_process.h"
#include "Emu/Memory/vm_ptr.h"
#include "Emu/System.h"
#include "Emu/IdManager.h"

#include "Crypto/unedat.h"
#include "Emu/Cell/ErrorCodes.h"
#include "Emu/Cell/PPUThread.h"
#include "sys_lwmutex.h"
#include "sys_lwcond.h"
#include "sys_mutex.h"
#include "sys_cond.h"
#include "sys_event.h"
#include "sys_event_flag.h"
#include "sys_interrupt.h"
#include "sys_memory.h"
#include "sys_mmapper.h"
#include "sys_prx.h"
#include "sys_overlay.h"
#include "sys_rwlock.h"
#include "sys_semaphore.h"
#include "sys_timer.h"
#include "sys_trace.h"
#include "sys_fs.h"
#include "sys_spu.h"

// Check all flags known to be related to extended permissions (TODO)
// It's possible anything which has root flags implicitly has debug perm as well
// But I haven't confirmed it.
bool ps3_process_info_t::debug_or_root() const
{
	return (ctrl_flags1 & (0xe << 28)) != 0;
}

bool ps3_process_info_t::has_root_perm() const
{
	return (ctrl_flags1 & (0xc << 28)) != 0;
}

bool ps3_process_info_t::has_debug_perm() const
{
	return (ctrl_flags1 & (0xa << 28)) != 0;
}

LOG_CHANNEL(sys_process);

ps3_process_info_t g_ps3_process_info;

s32 process_getpid()
{
	// TODO: get current process id
	return 1;
}

s32 sys_process_getpid()
{
	sys_process.trace("sys_process_getpid() -> 1");
	return process_getpid();
}

s32 sys_process_getppid()
{
	sys_process.todo("sys_process_getppid() -> 0");
	return 0;
}

template <typename T, typename Get>
u32 idm_get_count()
{
	return idm::select<T, Get>([&](u32, Get&) {});
}

error_code sys_process_get_number_of_object(u32 object, vm::ptr<u32> nump)
{
	sys_process.error("sys_process_get_number_of_object(object=0x%x, nump=*0x%x)", object, nump);

	switch(object)
	{
	case SYS_MEM_OBJECT: *nump = idm_get_count<lv2_obj, lv2_memory>(); break;
	case SYS_MUTEX_OBJECT: *nump = idm_get_count<lv2_obj, lv2_mutex>(); break;
	case SYS_COND_OBJECT: *nump = idm_get_count<lv2_obj, lv2_cond>(); break;
	case SYS_RWLOCK_OBJECT: *nump = idm_get_count<lv2_obj, lv2_rwlock>(); break;
	case SYS_INTR_TAG_OBJECT: *nump = idm_get_count<lv2_obj, lv2_int_tag>(); break;
	case SYS_INTR_SERVICE_HANDLE_OBJECT: *nump = idm_get_count<lv2_obj, lv2_int_serv>(); break;
	case SYS_EVENT_QUEUE_OBJECT: *nump = idm_get_count<lv2_obj, lv2_event_queue>(); break;
	case SYS_EVENT_PORT_OBJECT: *nump = idm_get_count<lv2_obj, lv2_event_port>(); break;
	case SYS_TRACE_OBJECT: sys_process.error("sys_process_get_number_of_object: object = SYS_TRACE_OBJECT"); *nump = 0; break;
	case SYS_SPUIMAGE_OBJECT: *nump = idm_get_count<lv2_obj, lv2_spu_image>(); break;
	case SYS_PRX_OBJECT: *nump = idm_get_count<lv2_obj, lv2_prx>(); break;
	case SYS_SPUPORT_OBJECT: sys_process.error("sys_process_get_number_of_object: object = SYS_SPUPORT_OBJECT"); *nump = 0; break;
	case SYS_OVERLAY_OBJECT: *nump = idm_get_count<lv2_obj, lv2_overlay>(); break;
	case SYS_LWMUTEX_OBJECT: *nump = idm_get_count<lv2_obj, lv2_lwmutex>(); break;
	case SYS_TIMER_OBJECT: *nump = idm_get_count<lv2_obj, lv2_timer>(); break;
	case SYS_SEMAPHORE_OBJECT: *nump = idm_get_count<lv2_obj, lv2_sema>(); break;
	case SYS_FS_FD_OBJECT: *nump = idm_get_count<lv2_fs_object, lv2_fs_object>(); break;
	case SYS_LWCOND_OBJECT: *nump = idm_get_count<lv2_obj, lv2_lwcond>(); break;
	case SYS_EVENT_FLAG_OBJECT: *nump = idm_get_count<lv2_obj, lv2_event_flag>(); break;

	default:
	{
		return CELL_EINVAL;
	}
	}

	return CELL_OK;
}

#include <set>

template <typename T, typename Get>
void idm_get_set(std::set<u32>& out)
{
	idm::select<T, Get>([&](u32 id, Get&)
	{
		out.emplace(id);
	});
}

static error_code process_get_id(u32 object, vm::ptr<u32> buffer, u32 size, vm::ptr<u32> set_size)
{
	std::set<u32> objects;

	switch (object)
	{
	case SYS_MEM_OBJECT: idm_get_set<lv2_obj, lv2_memory>(objects); break;
	case SYS_MUTEX_OBJECT: idm_get_set<lv2_obj, lv2_mutex>(objects); break;
	case SYS_COND_OBJECT: idm_get_set<lv2_obj, lv2_cond>(objects); break;
	case SYS_RWLOCK_OBJECT: idm_get_set<lv2_obj, lv2_rwlock>(objects); break;
	case SYS_INTR_TAG_OBJECT: idm_get_set<lv2_obj, lv2_int_tag>(objects); break;
	case SYS_INTR_SERVICE_HANDLE_OBJECT: idm_get_set<lv2_obj, lv2_int_serv>(objects); break;
	case SYS_EVENT_QUEUE_OBJECT: idm_get_set<lv2_obj, lv2_event_queue>(objects); break;
	case SYS_EVENT_PORT_OBJECT: idm_get_set<lv2_obj, lv2_event_port>(objects); break;
	case SYS_TRACE_OBJECT: fmt::throw_exception("SYS_TRACE_OBJECT" HERE);
	case SYS_SPUIMAGE_OBJECT: idm_get_set<lv2_obj, lv2_spu_image>(objects); break;
	case SYS_PRX_OBJECT: idm_get_set<lv2_obj, lv2_prx>(objects); break;
	case SYS_OVERLAY_OBJECT: idm_get_set<lv2_obj, lv2_overlay>(objects); break;
	case SYS_LWMUTEX_OBJECT: idm_get_set<lv2_obj, lv2_lwmutex>(objects); break;
	case SYS_TIMER_OBJECT: idm_get_set<lv2_obj, lv2_timer>(objects); break;
	case SYS_SEMAPHORE_OBJECT: idm_get_set<lv2_obj, lv2_sema>(objects); break;
	case SYS_FS_FD_OBJECT: idm_get_set<lv2_fs_object, lv2_fs_object>(objects); break;
	case SYS_LWCOND_OBJECT: idm_get_set<lv2_obj, lv2_lwcond>(objects); break;
	case SYS_EVENT_FLAG_OBJECT: idm_get_set<lv2_obj, lv2_event_flag>(objects); break;
	case SYS_SPUPORT_OBJECT: fmt::throw_exception("SYS_SPUPORT_OBJECT" HERE);
	default:
	{
		return CELL_EINVAL;
	}
	}

	u32 i = 0;

	// NOTE: Treats negative and 0 values as 1 due to signed checks and "do-while" behavior of fw
	for (auto id = objects.begin(); i < std::max<s32>(size, 1) + 0u && id != objects.end(); id++, i++)
	{
		buffer[i] = *id;
	}

	*set_size = i;

	return CELL_OK;
}

error_code sys_process_get_id(u32 object, vm::ptr<u32> buffer, u32 size, vm::ptr<u32> set_size)
{
	sys_process.error("sys_process_get_id(object=0x%x, buffer=*0x%x, size=%d, set_size=*0x%x)", object, buffer, size, set_size);

	if (object == SYS_SPUPORT_OBJECT)
	{
		// Unallowed for this syscall
		return CELL_EINVAL;
	}

	return process_get_id(object, buffer, size, set_size);
}

error_code sys_process_get_id2(u32 object, vm::ptr<u32> buffer, u32 size, vm::ptr<u32> set_size)
{
	sys_process.error("sys_process_get_id2(object=0x%x, buffer=*0x%x, size=%d, set_size=*0x%x)", object, buffer, size, set_size);

	if (!g_ps3_process_info.has_root_perm())
	{
		// This syscall is more capable than sys_process_get_id but also needs a root perm check
		return CELL_ENOSYS;
	}

	return process_get_id(object, buffer, size, set_size);
}

error_code process_is_spu_lock_line_reservation_address(u32 addr, u64 flags)
{
	if (!flags || flags & ~(SYS_MEMORY_ACCESS_RIGHT_SPU_THR | SYS_MEMORY_ACCESS_RIGHT_RAW_SPU))
	{
		return CELL_EINVAL;
	}

	// TODO
	return CELL_OK;
}

error_code sys_process_is_spu_lock_line_reservation_address(u32 addr, u64 flags)
{
	sys_process.warning("sys_process_is_spu_lock_line_reservation_address(addr=0x%x, flags=0x%llx)", addr, flags);

	return process_is_spu_lock_line_reservation_address(addr, flags);
}

error_code _sys_process_get_paramsfo(vm::ptr<char> buffer)
{
	sys_process.warning("_sys_process_get_paramsfo(buffer=0x%x)", buffer);

	if (!Emu.GetTitleID().length())
	{
		return CELL_ENOENT;
	}

	memset(buffer.get_ptr(), 0, 0x40);
	memcpy(buffer.get_ptr() + 1, Emu.GetTitleID().c_str(), std::min<size_t>(Emu.GetTitleID().length(), 9));

	return CELL_OK;
}

s32 process_get_sdk_version(u32 pid, s32& ver)
{
	// get correct SDK version for selected pid
	ver = g_ps3_process_info.sdk_ver;

	return CELL_OK;
}

error_code sys_process_get_sdk_version(u32 pid, vm::ptr<s32> version)
{
	sys_process.warning("sys_process_get_sdk_version(pid=0x%x, version=*0x%x)", pid, version);

	s32 sdk_ver;
	const s32 ret = process_get_sdk_version(pid, sdk_ver);
	if (ret != CELL_OK)
	{
		return CellError{ret + 0u}; // error code
	}
	else
	{
		*version = sdk_ver;
		return CELL_OK;
	}
}

error_code sys_process_kill(u32 pid)
{
	sys_process.todo("sys_process_kill(pid=0x%x)", pid);
	return CELL_OK;
}

error_code sys_process_wait_for_child(u32 pid, vm::ptr<u32> status, u64 unk)
{
	sys_process.todo("sys_process_wait_for_child(pid=0x%x, status=*0x%x, unk=0x%llx", pid, status, unk);

	return CELL_OK;
}

error_code sys_process_wait_for_child2(u64 unk1, u64 unk2, u64 unk3, u64 unk4, u64 unk5, u64 unk6)
{
	sys_process.todo("sys_process_wait_for_child2(unk1=0x%llx, unk2=0x%llx, unk3=0x%llx, unk4=0x%llx, unk5=0x%llx, unk6=0x%llx)",
		unk1, unk2, unk3, unk4, unk5, unk6);
	return CELL_OK;
}

error_code sys_process_get_status(u64 unk)
{
	sys_process.todo("sys_process_get_status(unk=0x%llx)", unk);
	//vm::write32(CPU.gpr[4], GetPPUThreadStatus(CPU));
	return CELL_OK;
}

error_code sys_process_detach_child(u64 unk)
{
	sys_process.todo("sys_process_detach_child(unk=0x%llx)", unk);
	return CELL_OK;
}

void _sys_process_exit(ppu_thread& ppu, s32 status, u32 arg2, u32 arg3)
{
	vm::temporary_unlock(ppu);

	sys_process.warning("_sys_process_exit(status=%d, arg2=0x%x, arg3=0x%x)", status, arg2, arg3);

	Emu.CallAfter([]()
	{
		sys_process.success("Process finished");
		Emu.Stop();
	});

	ppu.state += cpu_flag::dbg_global_stop;
}

void _sys_process_exit2(ppu_thread& ppu, s32 status, vm::ptr<sys_exit2_param> arg, u32 arg_size, u32 arg4)
{
	sys_process.warning("_sys_process_exit2(status=%d, arg=*0x%x, arg_size=0x%x, arg4=0x%x)", status, arg, arg_size, arg4);

	auto pstr = +arg->args;

	std::vector<std::string> argv;
	std::vector<std::string> envp;

	while (auto ptr = *pstr++)
	{
		argv.emplace_back(ptr.get_ptr());
		sys_process.notice(" *** arg: %s", ptr);
	}

	while (auto ptr = *pstr++)
	{
		envp.emplace_back(ptr.get_ptr());
		sys_process.notice(" *** env: %s", ptr);
	}

	std::vector<u8> data;

	if (arg_size > 0x1030)
	{
		data.resize(0x1000);
		std::memcpy(data.data(), vm::base(arg.addr() + arg_size - 0x1000), 0x1000);
	}

	if (argv.empty())
	{
		return _sys_process_exit(ppu, status, 0, 0);
	}

	// TODO: set prio, flags

	std::string path = vfs::get(argv[0]);
	std::string hdd1 = vfs::get("/dev_hdd1/");
	std::string disc;

	if (Emu.GetCat() == "DG" || Emu.GetCat() == "GD")
		disc = vfs::get("/dev_bdvd/");
	if (disc.empty() && Emu.GetTitleID().size())
		disc = vfs::get(Emu.GetDir());

	vm::temporary_unlock(ppu);

	Emu.CallAfter([path = std::move(path), argv = std::move(argv), envp = std::move(envp), data = std::move(data), disc = std::move(disc), hdd1 = std::move(hdd1), klic = g_fxo->get<loaded_npdrm_keys>()->devKlic]() mutable
	{
		sys_process.success("Process finished -> %s", argv[0]);
		Emu.SetForceBoot(true);
		Emu.Stop();
		Emu.argv = std::move(argv);
		Emu.envp = std::move(envp);
		Emu.data = std::move(data);
		Emu.disc = std::move(disc);
		Emu.hdd1 = std::move(hdd1);

		if (klic != std::array<u8, 16>{})
		{
			Emu.klic.assign(klic.begin(), klic.end());
		}

		Emu.SetForceBoot(true);
		Emu.BootGame(path, "", true);
	});

	ppu.state += cpu_flag::dbg_global_stop;
}

error_code sys_process_spawns_a_self2(vm::ptr<u32> pid, u32 primary_prio, u64 flags, vm::ptr<void> stack, u32 stack_size, u32 mem_id, vm::ptr<void> param_sfo, vm::ptr<void> dbg_data)
{
	sys_process.todo("sys_process_spawns_a_self2(pid=*0x%x, primary_prio=0x%x, flags=0x%llx, stack=*0x%x, stack_size=0x%x, mem_id=0x%x, param_sfo=*0x%x, dbg_data=*0x%x"
		, pid, primary_prio, flags, stack, stack_size, mem_id, param_sfo, dbg_data);

	return CELL_OK;
}
