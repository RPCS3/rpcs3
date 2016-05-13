#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/IdManager.h"

#include "Emu/Cell/ErrorCodes.h"
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
#include "sys_rwlock.h"
#include "sys_semaphore.h"
#include "sys_timer.h"
#include "sys_trace.h"
#include "sys_fs.h"
#include "sys_process.h"

#include <thread>

logs::channel sys_process("sys_process", logs::level::notice);

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

s32 sys_process_exit(s32 status)
{
	sys_process.warning("sys_process_exit(status=0x%x)", status);

	LV2_LOCK;

	CHECK_EMU_STATUS;
	
	sys_process.success("Process finished");

	Emu.CallAfter([]()
	{
		Emu.Stop();
	});

	while (true)
	{
		CHECK_EMU_STATUS;

		std::this_thread::sleep_for(1ms);
	}

	return CELL_OK;
}

s32 sys_process_get_number_of_object(u32 object, vm::ptr<u32> nump)
{
	sys_process.error("sys_process_get_number_of_object(object=0x%x, nump=*0x%x)", object, nump);

	switch(object)
	{
	case SYS_MEM_OBJECT: *nump = idm::get_count<lv2_memory_t>(); break;
	case SYS_MUTEX_OBJECT: *nump = idm::get_count<lv2_mutex_t>(); break;
	case SYS_COND_OBJECT: *nump = idm::get_count<lv2_cond_t>(); break;
	case SYS_RWLOCK_OBJECT: *nump = idm::get_count<lv2_rwlock_t>(); break;
	case SYS_INTR_TAG_OBJECT: *nump = idm::get_count<lv2_int_tag_t>(); break;
	case SYS_INTR_SERVICE_HANDLE_OBJECT: *nump = idm::get_count<lv2_int_serv_t>(); break;
	case SYS_EVENT_QUEUE_OBJECT: *nump = idm::get_count<lv2_event_queue_t>(); break;
	case SYS_EVENT_PORT_OBJECT: *nump = idm::get_count<lv2_event_port_t>(); break;
	case SYS_TRACE_OBJECT: throw EXCEPTION("SYS_TRACE_OBJECT");
	case SYS_SPUIMAGE_OBJECT: throw EXCEPTION("SYS_SPUIMAGE_OBJECT");
	case SYS_PRX_OBJECT: *nump = idm::get_count<lv2_prx_t>(); break;
	case SYS_SPUPORT_OBJECT: throw EXCEPTION("SYS_SPUPORT_OBJECT");
	case SYS_LWMUTEX_OBJECT: *nump = idm::get_count<lv2_lwmutex_t>(); break;
	case SYS_TIMER_OBJECT: *nump = idm::get_count<lv2_timer_t>(); break;
	case SYS_SEMAPHORE_OBJECT: *nump = idm::get_count<lv2_sema_t>(); break;
	case SYS_FS_FD_OBJECT: *nump = idm::get_count<lv2_fs_object_t>(); break;
	case SYS_LWCOND_OBJECT: *nump = idm::get_count<lv2_lwcond_t>(); break;
	case SYS_EVENT_FLAG_OBJECT: *nump = idm::get_count<lv2_event_flag_t>(); break;

	default:
	{
		return CELL_EINVAL;
	}
	}

	return CELL_OK;
}

#include <set>

template<typename T>
void idm_get_set(std::set<u32>& out)
{
	idm::select<T>([&](u32 id, T&)
	{
		out.emplace(id);
	});
}

s32 sys_process_get_id(u32 object, vm::ptr<u32> buffer, u32 size, vm::ptr<u32> set_size)
{
	sys_process.error("sys_process_get_id(object=0x%x, buffer=*0x%x, size=%d, set_size=*0x%x)", object, buffer, size, set_size);

	std::set<u32> objects;

	switch (object)
	{
	case SYS_MEM_OBJECT: idm_get_set<lv2_memory_t>(objects); break;
	case SYS_MUTEX_OBJECT: idm_get_set<lv2_mutex_t>(objects); break;
	case SYS_COND_OBJECT: idm_get_set<lv2_cond_t>(objects); break;
	case SYS_RWLOCK_OBJECT: idm_get_set<lv2_rwlock_t>(objects); break;
	case SYS_INTR_TAG_OBJECT: idm_get_set<lv2_int_tag_t>(objects); break;
	case SYS_INTR_SERVICE_HANDLE_OBJECT: idm_get_set<lv2_int_serv_t>(objects); break;
	case SYS_EVENT_QUEUE_OBJECT: idm_get_set<lv2_event_queue_t>(objects); break;
	case SYS_EVENT_PORT_OBJECT: idm_get_set<lv2_event_port_t>(objects); break;
	case SYS_TRACE_OBJECT: throw EXCEPTION("SYS_TRACE_OBJECT");
	case SYS_SPUIMAGE_OBJECT: throw EXCEPTION("SYS_SPUIMAGE_OBJECT");
	case SYS_PRX_OBJECT: idm_get_set<lv2_prx_t>(objects); break;
	case SYS_SPUPORT_OBJECT: throw EXCEPTION("SYS_SPUPORT_OBJECT");
	case SYS_LWMUTEX_OBJECT: idm_get_set<lv2_lwmutex_t>(objects); break;
	case SYS_TIMER_OBJECT: idm_get_set<lv2_timer_t>(objects); break;
	case SYS_SEMAPHORE_OBJECT: idm_get_set<lv2_sema_t>(objects); break;
	case SYS_FS_FD_OBJECT: idm_get_set<lv2_fs_object_t>(objects); break;
	case SYS_LWCOND_OBJECT: idm_get_set<lv2_lwcond_t>(objects); break;
	case SYS_EVENT_FLAG_OBJECT: idm_get_set<lv2_event_flag_t>(objects); break;

	default:
	{
		return CELL_EINVAL;
	}
	}

	u32 i = 0;

	for (auto id = objects.begin(); i < size && id != objects.end(); id++, i++)
	{
		buffer[i] = *id;
	}

	*set_size = i;

	return CELL_OK;
}

s32 process_is_spu_lock_line_reservation_address(u32 addr, u64 flags)
{
	if (!flags || flags & ~(SYS_MEMORY_ACCESS_RIGHT_SPU_THR | SYS_MEMORY_ACCESS_RIGHT_RAW_SPU))
	{
		return CELL_EINVAL;
	}

	// TODO
	return CELL_OK;
}

s32 sys_process_is_spu_lock_line_reservation_address(u32 addr, u64 flags)
{
	sys_process.warning("sys_process_is_spu_lock_line_reservation_address(addr=0x%x, flags=0x%llx)", addr, flags);

	return process_is_spu_lock_line_reservation_address(addr, flags);
}

s32 _sys_process_get_paramsfo(vm::ptr<char> buffer)
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
	ver = Emu.GetSDKVersion();

	return CELL_OK;
}

s32 sys_process_get_sdk_version(u32 pid, vm::ptr<s32> version)
{
	sys_process.warning("sys_process_get_sdk_version(pid=0x%x, version=*0x%x)", pid, version);

	s32 sdk_ver;
	s32 ret = process_get_sdk_version(pid, sdk_ver);
	if (ret != CELL_OK)
	{
		return ret; // error code
	}
	else
	{
		*version = sdk_ver;
		return CELL_OK;
	}
}

s32 sys_process_kill(u32 pid)
{
	sys_process.todo("sys_process_kill(pid=0x%x)", pid);
	return CELL_OK;
}

s32 sys_process_wait_for_child(u32 pid, vm::ptr<u32> status, u64 unk)
{
	sys_process.todo("sys_process_wait_for_child(pid=0x%x, status=*0x%x, unk=0x%llx", pid, status, unk);

	return CELL_OK;
}

s32 sys_process_wait_for_child2(u64 unk1, u64 unk2, u64 unk3, u64 unk4, u64 unk5, u64 unk6)
{
	sys_process.todo("sys_process_wait_for_child2(unk1=0x%llx, unk2=0x%llx, unk3=0x%llx, unk4=0x%llx, unk5=0x%llx, unk6=0x%llx)",
		unk1, unk2, unk3, unk4, unk5, unk6);
	return CELL_OK;
}

s32 sys_process_get_status(u64 unk)
{
	sys_process.todo("sys_process_get_status(unk=0x%llx)", unk);
	//vm::write32(CPU.GPR[4], GetPPUThreadStatus(CPU));
	return CELL_OK;
}

s32 sys_process_detach_child(u64 unk)
{
	sys_process.todo("sys_process_detach_child(unk=0x%llx)", unk);
	return CELL_OK;
}
