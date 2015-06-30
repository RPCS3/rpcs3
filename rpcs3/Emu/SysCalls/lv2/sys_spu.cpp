#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/IdManager.h"
#include "Emu/SysCalls/SysCalls.h"

#include "Emu/CPU/CPUThreadManager.h"
#include "Emu/Cell/RawSPUThread.h"
#include "Emu/FS/vfsStreamMemory.h"
#include "Emu/FS/vfsFile.h"
#include "Loader/ELF32.h"
#include "Crypto/unself.h"
#include "sys_event.h"
#include "sys_spu.h"

SysCallBase sys_spu("sys_spu");

void LoadSpuImage(vfsStream& stream, u32& spu_ep, u32 addr)
{
	loader::handlers::elf32 h;
	h.init(stream);
	h.load_data(addr);
	spu_ep = h.m_ehdr.data_be.e_entry;
}

u32 LoadSpuImage(vfsStream& stream, u32& spu_ep)
{
	const u32 alloc_size = 256 * 1024;
	u32 spu_offset = (u32)vm::alloc(alloc_size, vm::main);

	LoadSpuImage(stream, spu_ep, spu_offset);
	return spu_offset;
}

s32 spu_image_import(sys_spu_image& img, u32 src, u32 type)
{
	vfsStreamMemory f(src);
	u32 entry;
	u32 offset = LoadSpuImage(f, entry);

	img.type = SYS_SPU_IMAGE_TYPE_USER;
	img.entry_point = entry;
	img.addr = offset; // TODO: writing actual segment info
	img.nsegs = 1; // wrong value

	return CELL_OK;
}

s32 sys_spu_initialize(u32 max_usable_spu, u32 max_raw_spu)
{
	sys_spu.Warning("sys_spu_initialize(max_usable_spu=%d, max_raw_spu=%d)", max_usable_spu, max_raw_spu);

	if (max_raw_spu > 5)
	{
		return CELL_EINVAL;
	}

	return CELL_OK;
}

s32 sys_spu_image_open(vm::ptr<sys_spu_image> img, vm::cptr<char> path)
{
	sys_spu.Warning("sys_spu_image_open(img_addr=0x%x, path_addr=0x%x [%s])", img.addr(), path.addr(), path.get_ptr());

	vfsFile f(path.get_ptr());
	if(!f.IsOpened())
	{
		sys_spu.Error("sys_spu_image_open error: '%s' not found!", path.get_ptr());
		return CELL_ENOENT;
	}

	SceHeader hdr;
	hdr.Load(f);

	if (hdr.CheckMagic())
	{
		sys_spu.Error("sys_spu_image_open error: '%s' is encrypted! Decrypt SELF and try again.", path.get_ptr());
		Emu.Pause();
		return CELL_ENOENT;
	}

	f.Seek(0);

	u32 entry;
	u32 offset = LoadSpuImage(f, entry);

	img->type = SYS_SPU_IMAGE_TYPE_USER;
	img->entry_point = entry;
	img->addr = offset; // TODO: writing actual segment info
	img->nsegs = 1; // wrong value

	return CELL_OK;
}

u32 spu_thread_initialize(u32 group_id, u32 spu_num, vm::ptr<sys_spu_image> img, const std::string& name, u32 option, u64 a1, u64 a2, u64 a3, u64 a4, std::function<void(SPUThread&)> task = nullptr)
{
	if (option)
	{
		sys_spu.Todo("Unsupported SPU Thread options (0x%x)", option);
	}

	const auto spu = Emu.GetIdManager().make_ptr<SPUThread>(name, spu_num, Memory.MainMem.AllocAlign(0x40000));

	spu->m_custom_task = task;

	const auto group = Emu.GetIdManager().get<spu_group_t>(group_id);

	spu->tg = group;
	group->threads[spu_num] = spu;
	group->args[spu_num] = { a1, a2, a3, a4 };
	group->images[spu_num] = img;

	u32 count = 0;

	for (auto& t : group->threads)
	{
		if (t)
		{
			count++;
		}
	}

	if (count >= group->num)
	{
		assert(count == group->num);
		group->state = SPU_THREAD_GROUP_STATUS_INITIALIZED;
	}
	
	return spu->GetId();
}

s32 sys_spu_thread_initialize(vm::ptr<u32> thread, u32 group_id, u32 spu_num, vm::ptr<sys_spu_image> img, vm::ptr<sys_spu_thread_attribute> attr, vm::ptr<sys_spu_thread_argument> arg)
{
	sys_spu.Warning("sys_spu_thread_initialize(thread=*0x%x, group=0x%x, spu_num=%d, img=*0x%x, attr=*0x%x, arg=*0x%x)", thread, group_id, spu_num, img, attr, arg);

	LV2_LOCK;

	const auto group = Emu.GetIdManager().get<spu_group_t>(group_id);

	if (!group)
	{
		return CELL_ESRCH;
	}

	if (spu_num >= group->threads.size())
	{
		return CELL_EINVAL;
	}
	
	if (group->threads[spu_num] || group->state != SPU_THREAD_GROUP_STATUS_NOT_INITIALIZED)
	{
		return CELL_EBUSY;
	}

	*thread = spu_thread_initialize(group_id, spu_num, img, attr->name ? std::string(attr->name.get_ptr(), attr->name_len) : "SPUThread", attr->option, arg->arg1, arg->arg2, arg->arg3, arg->arg4);
	return CELL_OK;
}

s32 sys_spu_thread_set_argument(u32 id, vm::ptr<sys_spu_thread_argument> arg)
{
	sys_spu.Warning("sys_spu_thread_set_argument(id=0x%x, arg=*0x%x)", id, arg);

	LV2_LOCK;

	const auto thread = Emu.GetIdManager().get<SPUThread>(id);

	if (!thread)
	{
		return CELL_ESRCH;
	}

	const auto group = thread->tg.lock();

	assert(thread->index < group->threads.size());

	group->args[thread->index].arg1 = arg->arg1;
	group->args[thread->index].arg2 = arg->arg2;
	group->args[thread->index].arg3 = arg->arg3;
	group->args[thread->index].arg4 = arg->arg4;

	return CELL_OK;
}

s32 sys_spu_thread_get_exit_status(u32 id, vm::ptr<u32> status)
{
	sys_spu.Warning("sys_spu_thread_get_exit_status(id=0x%x, status=*0x%x)", id, status);

	LV2_LOCK;

	const auto thread = Emu.GetIdManager().get<SPUThread>(id);

	if (!thread)
	{
		return CELL_ESRCH;
	}

	// TODO: check CELL_ESTAT condition

	*status = thread->ch_out_mbox.pop_uncond();

	return CELL_OK;
}

s32 sys_spu_thread_group_create(vm::ptr<u32> id, u32 num, s32 prio, vm::ptr<sys_spu_thread_group_attribute> attr)
{
	sys_spu.Warning("sys_spu_thread_group_create(id=*0x%x, num=%d, prio=%d, attr=*0x%x)", id, num, prio, attr);

	// TODO: max num value should be affected by sys_spu_initialize() settings

	if (!num || num > 6 || prio < 16 || prio > 255)
	{
		return CELL_EINVAL;
	}

	if (attr->type.data())
	{
		sys_spu.Todo("Unsupported SPU Thread Group type (0x%x)", attr->type);
	}

	*id = Emu.GetIdManager().make<spu_group_t>(std::string{ attr->name.get_ptr(), attr->nsize - 1 }, num, prio, attr->type, attr->ct);

	return CELL_OK;
}

s32 sys_spu_thread_group_destroy(u32 id)
{
	sys_spu.Warning("sys_spu_thread_group_destroy(id=0x%x)", id);

	LV2_LOCK;

	const auto group = Emu.GetIdManager().get<spu_group_t>(id);

	if (!group)
	{
		return CELL_ESRCH;
	}

	if (group->state > SPU_THREAD_GROUP_STATUS_INITIALIZED)
	{
		return CELL_EBUSY;
	}

	// clear threads
	for (auto& t : group->threads)
	{
		if (t)
		{
			auto& spu = static_cast<SPUThread&>(*t);

			Memory.MainMem.Free(spu.offset);
			Emu.GetIdManager().remove<SPUThread>(spu.GetId());

			t.reset();
		}
	}

	group->state = SPU_THREAD_GROUP_STATUS_NOT_INITIALIZED; // hack
	Emu.GetIdManager().remove<spu_group_t>(id);

	return CELL_OK;
}

s32 sys_spu_thread_group_start(u32 id)
{
	sys_spu.Warning("sys_spu_thread_group_start(id=0x%x)", id);

	LV2_LOCK;

	const auto group = Emu.GetIdManager().get<spu_group_t>(id);

	if (!group)
	{
		return CELL_ESRCH;
	}

	if (group->state != SPU_THREAD_GROUP_STATUS_INITIALIZED)
	{
		return CELL_ESTAT;
	}

	// SPU_THREAD_GROUP_STATUS_READY state is not used

	group->state = SPU_THREAD_GROUP_STATUS_RUNNING;
	group->join_state = 0;

	for (auto& t : group->threads)
	{
		if (t)
		{
			auto& spu = static_cast<SPUThread&>(*t);

			assert(spu.index < group->threads.size());
			auto& args = group->args[spu.index];
			auto& image = group->images[spu.index];

			// Copy SPU image:
			// TODO: use segment info
			memcpy(vm::get_ptr<void>(spu.offset), vm::get_ptr<void>(image->addr), 256 * 1024);

			spu.Run();
			spu.PC = image->entry_point;
			spu.GPR[3] = u128::from64(0, args.arg1);
			spu.GPR[4] = u128::from64(0, args.arg2);
			spu.GPR[5] = u128::from64(0, args.arg3);
			spu.GPR[6] = u128::from64(0, args.arg4);

			spu.status.exchange(SPU_STATUS_RUNNING);
		}
	}

	// because SPU_THREAD_GROUP_STATUS_READY is not possible, run event is delivered immediately

	group->send_run_event(lv2_lock, id, 0, 0); // TODO: check data2 and data3

	for (auto& t : group->threads)
	{
		if (t)
		{
			t->Exec();
		}
	}

	return CELL_OK;
}

s32 sys_spu_thread_group_suspend(u32 id)
{
	sys_spu.Log("sys_spu_thread_group_suspend(id=0x%x)", id);

	LV2_LOCK;

	const auto group = Emu.GetIdManager().get<spu_group_t>(id);

	if (!group)
	{
		return CELL_ESRCH;
	}

	if (group->type & SYS_SPU_THREAD_GROUP_TYPE_EXCLUSIVE_NON_CONTEXT) // this check may be inaccurate
	{
		return CELL_EINVAL;
	}

	if (group->state <= SPU_THREAD_GROUP_STATUS_INITIALIZED || group->state == SPU_THREAD_GROUP_STATUS_STOPPED)
	{
		return CELL_ESTAT;
	}

	// SPU_THREAD_GROUP_STATUS_READY state is not used

	if (group->state == SPU_THREAD_GROUP_STATUS_RUNNING)
	{
		group->state = SPU_THREAD_GROUP_STATUS_SUSPENDED;
	}
	else if (group->state == SPU_THREAD_GROUP_STATUS_WAITING)
	{
		group->state = SPU_THREAD_GROUP_STATUS_WAITING_AND_SUSPENDED;
	}
	else if (group->state == SPU_THREAD_GROUP_STATUS_SUSPENDED || group->state == SPU_THREAD_GROUP_STATUS_WAITING_AND_SUSPENDED)
	{
		return CELL_OK; // probably, nothing to do there
	}
	else
	{
		return CELL_ESTAT;
	}

	for (auto& t : group->threads)
	{
		if (t)
		{
			t->Sleep(); // trigger m_state check
		}
	}

	return CELL_OK;
}

s32 sys_spu_thread_group_resume(u32 id)
{
	sys_spu.Log("sys_spu_thread_group_resume(id=0x%x)", id);

	LV2_LOCK;

	const auto group = Emu.GetIdManager().get<spu_group_t>(id);

	if (!group)
	{
		return CELL_ESRCH;
	}

	if (group->type & SYS_SPU_THREAD_GROUP_TYPE_EXCLUSIVE_NON_CONTEXT) // this check may be inaccurate
	{
		return CELL_EINVAL;
	}

	// SPU_THREAD_GROUP_STATUS_READY state is not used

	if (group->state == SPU_THREAD_GROUP_STATUS_SUSPENDED)
	{
		group->state = SPU_THREAD_GROUP_STATUS_RUNNING;
	}
	else if (group->state == SPU_THREAD_GROUP_STATUS_WAITING_AND_SUSPENDED)
	{
		group->state = SPU_THREAD_GROUP_STATUS_WAITING;
		return CELL_OK; // probably, nothing to do there
	}
	else
	{
		return CELL_ESTAT;
	}

	for (auto& t : group->threads)
	{
		if (t)
		{
			t->Awake(); // trigger m_state check
		}
	}

	return CELL_OK;
}

s32 sys_spu_thread_group_yield(u32 id)
{
	sys_spu.Log("sys_spu_thread_group_yield(id=0x%x)", id);

	LV2_LOCK;

	const auto group = Emu.GetIdManager().get<spu_group_t>(id);

	if (!group)
	{
		return CELL_ESRCH;
	}

	if (group->state != SPU_THREAD_GROUP_STATUS_RUNNING)
	{
		return CELL_ESTAT;
	}

	// SPU_THREAD_GROUP_STATUS_READY state is not used, so this function does nothing

	return CELL_OK;
}

s32 sys_spu_thread_group_terminate(u32 id, s32 value)
{
	sys_spu.Warning("sys_spu_thread_group_terminate(id=0x%x, value=0x%x)", id, value);

	LV2_LOCK;

	// seems the id can be either SPU Thread Group or SPU Thread
	const auto thread = Emu.GetIdManager().get<SPUThread>(id);
	auto group = Emu.GetIdManager().get<spu_group_t>(id);

	if (!group && !thread)
	{
		return CELL_ESRCH;
	}

	auto& spu = static_cast<SPUThread&>(*thread);

	if (thread)
	{
		if (group)
		{
			throw __FUNCTION__;
		}

		group = spu.tg.lock();

		for (auto& t : group->threads)
		{
			// find primary (?) thread and compare it with the one specified
			if (t)
			{
				if (t == thread)
				{
					break;
				}
				else
				{
					return CELL_EPERM;
				}
			}
		}
	}

	if ((group->state <= SPU_THREAD_GROUP_STATUS_INITIALIZED) || (group->state == SPU_THREAD_GROUP_STATUS_WAITING))
	{
		return CELL_ESTAT;
	}

	for (auto& t : group->threads)
	{
		if (t)
		{
			auto& spu = static_cast<SPUThread&>(*t);

			spu.status.exchange(SPU_STATUS_STOPPED);
			spu.Stop();
		}
	}

	group->state = SPU_THREAD_GROUP_STATUS_INITIALIZED;
	group->exit_status = value;
	group->join_state |= SPU_TGJSF_TERMINATED;
	group->join_cv.notify_one();

	return CELL_OK;
}

s32 sys_spu_thread_group_join(u32 id, vm::ptr<u32> cause, vm::ptr<u32> status)
{
	sys_spu.Warning("sys_spu_thread_group_join(id=0x%x, cause=*0x%x, status=*0x%x)", id, cause, status);

	LV2_LOCK;

	const auto group = Emu.GetIdManager().get<spu_group_t>(id);

	if (!group)
	{
		return CELL_ESRCH;
	}

	if (group->state < SPU_THREAD_GROUP_STATUS_INITIALIZED)
	{
		return CELL_ESTAT;
	}

	if (group->join_state.fetch_or(SPU_TGJSF_IS_JOINING) & SPU_TGJSF_IS_JOINING)
	{
		// another PPU thread is joining this thread group
		return CELL_EBUSY;
	}

	while ((group->join_state & ~SPU_TGJSF_IS_JOINING) == 0)
	{
		bool stopped = true;

		for (auto& t : group->threads)
		{
			if (t)
			{
				auto& spu = static_cast<SPUThread&>(*t);

				if ((spu.status.load() & SPU_STATUS_STOPPED_BY_STOP) == 0)
				{
					stopped = false;
					break;
				}
			}
		}

		if (stopped)
		{
			break;
		}

		if (Emu.IsStopped())
		{
			sys_spu.Warning("sys_spu_thread_group_join(id=0x%x) aborted", id);
			return CELL_OK;
		}

		group->join_cv.wait_for(lv2_lock, std::chrono::milliseconds(1));
	}

	switch (group->join_state & ~SPU_TGJSF_IS_JOINING)
	{
	case 0:
	{
		if (cause) *cause = SYS_SPU_THREAD_GROUP_JOIN_ALL_THREADS_EXIT;
		break;
	}
	case SPU_TGJSF_GROUP_EXIT:
	{
		if (cause) *cause = SYS_SPU_THREAD_GROUP_JOIN_GROUP_EXIT;
		break;
	}
	case SPU_TGJSF_TERMINATED:
	{
		if (cause) *cause = SYS_SPU_THREAD_GROUP_JOIN_TERMINATED;
		break;
	}
	default: throw __FUNCTION__;
	}

	if (status)
	{
		*status = group->exit_status;
	}

	group->join_state &= ~SPU_TGJSF_IS_JOINING;
	group->state = SPU_THREAD_GROUP_STATUS_INITIALIZED; // hack
	return CELL_OK;
}

s32 sys_spu_thread_write_ls(u32 id, u32 address, u64 value, u32 type)
{
	sys_spu.Log("sys_spu_thread_write_ls(id=0x%x, address=0x%x, value=0x%llx, type=%d)", id, address, value, type);

	LV2_LOCK;

	const auto thread = Emu.GetIdManager().get<SPUThread>(id);

	if (!thread)
	{
		return CELL_ESRCH;
	}

	if (address >= 0x40000 || address + type > 0x40000 || address % type) // check range and alignment
	{
		return CELL_EINVAL;
	}

	const auto group = thread->tg.lock();

	if ((group->state < SPU_THREAD_GROUP_STATUS_WAITING) || (group->state > SPU_THREAD_GROUP_STATUS_RUNNING))
	{
		return CELL_ESTAT;
	}

	switch (type)
	{
	case 1: thread->write8(address, (u8)value); break;
	case 2: thread->write16(address, (u16)value); break;
	case 4: thread->write32(address, (u32)value); break;
	case 8: thread->write64(address, value); break;
	default: return CELL_EINVAL;
	}

	return CELL_OK;
}

s32 sys_spu_thread_read_ls(u32 id, u32 address, vm::ptr<u64> value, u32 type)
{
	sys_spu.Log("sys_spu_thread_read_ls(id=0x%x, address=0x%x, value=*0x%x, type=%d)", id, address, value, type);

	LV2_LOCK;

	const auto thread = Emu.GetIdManager().get<SPUThread>(id);

	if (!thread)
	{
		return CELL_ESRCH;
	}

	if (address >= 0x40000 || address + type > 0x40000 || address % type) // check range and alignment
	{
		return CELL_EINVAL;
	}

	const auto group = thread->tg.lock();

	if ((group->state < SPU_THREAD_GROUP_STATUS_WAITING) || (group->state > SPU_THREAD_GROUP_STATUS_RUNNING))
	{
		return CELL_ESTAT;
	}

	switch (type)
	{
	case 1: *value = thread->read8(address); break;
	case 2: *value = thread->read16(address); break;
	case 4: *value = thread->read32(address); break;
	case 8: *value = thread->read64(address); break;
	default: return CELL_EINVAL;
	}

	return CELL_OK;
}

s32 sys_spu_thread_write_spu_mb(u32 id, u32 value)
{
	sys_spu.Warning("sys_spu_thread_write_spu_mb(id=0x%x, value=0x%x)", id, value);

	LV2_LOCK;

	const auto thread = Emu.GetIdManager().get<SPUThread>(id);

	if (!thread)
	{
		return CELL_ESRCH;
	}

	const auto group = thread->tg.lock();

	if ((group->state < SPU_THREAD_GROUP_STATUS_WAITING) || (group->state > SPU_THREAD_GROUP_STATUS_RUNNING))
	{
		return CELL_ESTAT;
	}

	thread->ch_in_mbox.push_uncond(value);

	return CELL_OK;
}

s32 sys_spu_thread_set_spu_cfg(u32 id, u64 value)
{
	sys_spu.Warning("sys_spu_thread_set_spu_cfg(id=0x%x, value=0x%x)", id, value);

	LV2_LOCK;

	const auto thread = Emu.GetIdManager().get<SPUThread>(id);

	if (!thread)
	{
		return CELL_ESRCH;
	}

	if (value > 3)
	{
		return CELL_EINVAL;
	}

	thread->snr_config = value;

	return CELL_OK;
}

s32 sys_spu_thread_get_spu_cfg(u32 id, vm::ptr<u64> value)
{
	sys_spu.Warning("sys_spu_thread_get_spu_cfg(id=0x%x, value=*0x%x)", id, value);

	LV2_LOCK;

	const auto thread = Emu.GetIdManager().get<SPUThread>(id);

	if (!thread)
	{
		return CELL_ESRCH;
	}

	*value = thread->snr_config;

	return CELL_OK;
}

s32 sys_spu_thread_write_snr(u32 id, u32 number, u32 value)
{
	sys_spu.Log("sys_spu_thread_write_snr(id=0x%x, number=%d, value=0x%x)", id, number, value);

	LV2_LOCK;

	const auto thread = Emu.GetIdManager().get<SPUThread>(id);

	if (!thread)
	{
		return CELL_ESRCH;
	}

	if (number > 1)
	{
		return CELL_EINVAL;
	}

	const auto group = thread->tg.lock();

	if ((group->state < SPU_THREAD_GROUP_STATUS_WAITING) || (group->state > SPU_THREAD_GROUP_STATUS_RUNNING))
	{
		return CELL_ESTAT;
	}

	thread->write_snr(number, value);

	return CELL_OK;
}

s32 sys_spu_thread_group_connect_event(u32 id, u32 eq, u32 et)
{
	sys_spu.Warning("sys_spu_thread_group_connect_event(id=0x%x, eq=0x%x, et=%d)", id, eq, et);

	LV2_LOCK;

	const auto group = Emu.GetIdManager().get<spu_group_t>(id);
	const auto queue = Emu.GetIdManager().get<lv2_event_queue_t>(eq);

	if (!group || !queue)
	{
		return CELL_ESRCH;
	}

	switch (et)
	{
	case SYS_SPU_THREAD_GROUP_EVENT_RUN:
	{
		if (!group->ep_run.expired())
		{
			return CELL_EBUSY;
		}
		
		group->ep_run = queue;
		break;
	}
	case SYS_SPU_THREAD_GROUP_EVENT_EXCEPTION:
	{
		if (!group->ep_exception.expired())
		{
			return CELL_EBUSY;
		}

		group->ep_exception = queue;
		break;
	}
	case SYS_SPU_THREAD_GROUP_EVENT_SYSTEM_MODULE:
	{
		if (!group->ep_sysmodule.expired())
		{
			return CELL_EBUSY;
		}

		group->ep_sysmodule = queue;
		break;
	}
	default:
	{
		sys_spu.Error("sys_spu_thread_group_connect_event(): unknown event type (%d)", et);
		return CELL_EINVAL;
	}
	}

	return CELL_OK;
}

s32 sys_spu_thread_group_disconnect_event(u32 id, u32 et)
{
	sys_spu.Warning("sys_spu_thread_group_disconnect_event(id=0x%x, et=%d)", id, et);

	LV2_LOCK;

	const auto group = Emu.GetIdManager().get<spu_group_t>(id);

	if (!group)
	{
		return CELL_ESRCH;
	}

	switch (et)
	{
	case SYS_SPU_THREAD_GROUP_EVENT_RUN:
	{
		if (group->ep_run.expired())
		{
			return CELL_ENOTCONN;
		}

		group->ep_run.reset();
		break;
	}
	case SYS_SPU_THREAD_GROUP_EVENT_EXCEPTION:
	{
		if (group->ep_exception.expired())
		{
			return CELL_ENOTCONN;
		}

		group->ep_exception.reset();
		break;
	}
	case SYS_SPU_THREAD_GROUP_EVENT_SYSTEM_MODULE:
	{
		if (group->ep_sysmodule.expired())
		{
			return CELL_ENOTCONN;
		}

		group->ep_sysmodule.reset();
		break;
	}
	default:
	{
		sys_spu.Error("sys_spu_thread_group_disconnect_event(): unknown event type (%d)", et);
		return CELL_EINVAL;
	}
	}

	return CELL_OK;
}

/*
SPU-Side functions:
s32 sys_spu_thread_receive_event(u32 spuq_num, mem32_t d1, mem32_t d2, mem32_t d3);
s32 sys_spu_thread_send_event(u8 spup, u24 data0, u32 data1);
s32 sys_spu_thread_throw_event(u8 spup, u24 data0, u32 data1);
s32 sys_spu_thread_tryreceive_event(u32 spuq_num, mem32_t d1, mem32_t d2, mem32_t d3);
*/

s32 sys_spu_thread_connect_event(u32 id, u32 eq, u32 et, u8 spup)
{
	sys_spu.Warning("sys_spu_thread_connect_event(id=0x%x, eq=0x%x, et=%d, spup=%d)", id, eq, et, spup);

	LV2_LOCK;

	const auto thread = Emu.GetIdManager().get<SPUThread>(id);
	const auto queue = Emu.GetIdManager().get<lv2_event_queue_t>(eq);

	if (!thread || !queue)
	{
		return CELL_ESRCH;
	}

	if (et != SYS_SPU_THREAD_EVENT_USER || spup > 63 || queue->type != SYS_PPU_QUEUE)
	{
		sys_spu.Error("sys_spu_thread_connect_event(): invalid arguments (et=%d, spup=%d, queue->type=%d)", et, spup, queue->type);
		return CELL_EINVAL;
	}

	auto& port = thread->spup[spup];

	if (!port.expired())
	{
		return CELL_EISCONN;
	}

	port = queue;

	return CELL_OK;
}

s32 sys_spu_thread_disconnect_event(u32 id, u32 et, u8 spup)
{
	sys_spu.Warning("sys_spu_thread_disconnect_event(id=0x%x, et=%d, spup=%d)", id, et, spup);

	LV2_LOCK;

	const auto thread = Emu.GetIdManager().get<SPUThread>(id);

	if (!thread)
	{
		return CELL_ESRCH;
	}

	if (et != SYS_SPU_THREAD_EVENT_USER || spup > 63)
	{
		sys_spu.Error("sys_spu_thread_disconnect_event(): invalid arguments (et=%d, spup=%d)", et, spup);
		return CELL_EINVAL;
	}

	auto& port = thread->spup[spup];

	if (port.expired())
	{
		return CELL_ENOTCONN;
	}

	port.reset();

	return CELL_OK;
}

s32 sys_spu_thread_bind_queue(u32 id, u32 spuq, u32 spuq_num)
{
	sys_spu.Warning("sys_spu_thread_bind_queue(id=0x%x, spuq=0x%x, spuq_num=0x%x)", id, spuq, spuq_num);

	LV2_LOCK;

	const auto thread = Emu.GetIdManager().get<SPUThread>(id);
	const auto queue = Emu.GetIdManager().get<lv2_event_queue_t>(spuq);

	if (!thread || !queue)
	{
		return CELL_ESRCH;
	}

	if (queue->type != SYS_SPU_QUEUE)
	{
		return CELL_EINVAL;
	}

	for (auto& v : thread->spuq)
	{
		if (auto q = v.second.lock())
		{
			if (v.first == spuq_num || q == queue)
			{
				return CELL_EBUSY;
			}
		}
	}

	for (auto& v : thread->spuq)
	{
		if (v.second.expired())
		{
			v.first = spuq_num;
			v.second = queue;

			return CELL_OK;
		}
	}

	return CELL_EAGAIN;
}

s32 sys_spu_thread_unbind_queue(u32 id, u32 spuq_num)
{
	sys_spu.Warning("sys_spu_thread_unbind_queue(id=0x%x, spuq_num=0x%x)", id, spuq_num);

	LV2_LOCK;

	const auto thread = Emu.GetIdManager().get<SPUThread>(id);

	if (!thread)
	{
		return CELL_ESRCH;
	}

	for (auto& v : thread->spuq)
	{
		if (v.first == spuq_num && !v.second.expired())
		{
			v.second.reset();

			return CELL_OK;
		}
	}

	return CELL_ESRCH;
}

s32 sys_spu_thread_group_connect_event_all_threads(u32 id, u32 eq, u64 req, vm::ptr<u8> spup)
{
	sys_spu.Warning("sys_spu_thread_group_connect_event_all_threads(id=0x%x, eq=0x%x, req=0x%llx, spup=*0x%x)", id, eq, req, spup);

	LV2_LOCK;

	const auto group = Emu.GetIdManager().get<spu_group_t>(id);
	const auto queue = Emu.GetIdManager().get<lv2_event_queue_t>(eq);

	if (!group || !queue)
	{
		return CELL_ESRCH;
	}

	if (!req)
	{
		return CELL_EINVAL;
	}

	if (group->state < SPU_THREAD_GROUP_STATUS_INITIALIZED)
	{
		return CELL_ESTAT;
	}

	u8 port = 0; // SPU Port number

	for (; port < 64; port++)
	{
		if (!(req & (1ull << port)))
		{
			continue;
		}

		bool found = true;

		for (auto& t : group->threads)
		{
			if (t)
			{
				auto& spu = static_cast<SPUThread&>(*t);

				if (!spu.spup[port].expired())
				{
					found = false;
					break;
				}
			}
		}

		if (found)
		{
			break;
		}
	}

	if (port == 64)
	{
		return CELL_EISCONN;
	}

	for (auto& t : group->threads)
	{
		if (t)
		{
			auto& spu = static_cast<SPUThread&>(*t);

			spu.spup[port] = queue;
		}
	}

	*spup = port;

	return CELL_OK;
}

s32 sys_spu_thread_group_disconnect_event_all_threads(u32 id, u8 spup)
{
	sys_spu.Warning("sys_spu_thread_group_disconnect_event_all_threads(id=0x%x, spup=%d)", id, spup);

	LV2_LOCK;

	const auto group = Emu.GetIdManager().get<spu_group_t>(id);

	if (!group)
	{
		return CELL_ESRCH;
	}

	if (spup > 63)
	{
		return CELL_EINVAL;
	}

	for (auto& t : group->threads)
	{
		if (t)
		{
			auto& spu = static_cast<SPUThread&>(*t);
			
			spu.spup[spup].reset();
		}
	}

	return CELL_OK;
}

s32 sys_raw_spu_create(vm::ptr<u32> id, vm::ptr<void> attr)
{
	sys_spu.Warning("sys_raw_spu_create(id=*0x%x, attr=*0x%x)", id, attr);

	LV2_LOCK;

	const auto thread = Emu.GetCPU().NewRawSPUThread();

	if (!thread)
	{
		return CELL_EAGAIN;
	}

	thread->Run();

	*id = thread->index;

	return CELL_OK;
}

s32 sys_raw_spu_destroy(u32 id)
{
	sys_spu.Warning("sys_raw_spu_destroy(id=%d)", id);

	LV2_LOCK;

	const auto thread = Emu.GetCPU().GetRawSPUThread(id);

	if (!thread)
	{
		return CELL_ESRCH;
	}

	// TODO: check if busy

	Emu.GetIdManager().remove<RawSPUThread>(thread->GetId());

	return CELL_OK;
}

s32 sys_raw_spu_create_interrupt_tag(u32 id, u32 class_id, u32 hwthread, vm::ptr<u32> intrtag)
{
	sys_spu.Warning("sys_raw_spu_create_interrupt_tag(id=%d, class_id=%d, hwthread=0x%x, intrtag=*0x%x)", id, class_id, hwthread, intrtag);

	if (class_id != 0 && class_id != 2)
	{
		return CELL_EINVAL;
	}

	const auto t = Emu.GetCPU().GetRawSPUThread(id);

	if (!t)
	{
		return CELL_ESRCH;
	}

	auto& spu = static_cast<RawSPUThread&>(*t);

	auto& tag = class_id ? spu.int2 : spu.int0;

	if (!tag.assigned.compare_and_swap_test(-1, 0))
	{
		return CELL_EAGAIN;
	}

	*intrtag = (id & 0xff) | (class_id << 8);

	return CELL_OK;
}

s32 sys_raw_spu_set_int_mask(u32 id, u32 class_id, u64 mask)
{
	sys_spu.Log("sys_raw_spu_set_int_mask(id=%d, class_id=%d, mask=0x%llx)", id, class_id, mask);

	if (class_id != 0 && class_id != 2)
	{
		return CELL_EINVAL;
	}

	const auto t = Emu.GetCPU().GetRawSPUThread(id);

	if (!t)
	{
		return CELL_ESRCH;
	}

	auto& spu = static_cast<RawSPUThread&>(*t);

	(class_id ? spu.int2 : spu.int0).mask.exchange(mask);

	return CELL_OK;
}

s32 sys_raw_spu_get_int_mask(u32 id, u32 class_id, vm::ptr<u64> mask)
{
	sys_spu.Log("sys_raw_spu_get_int_mask(id=%d, class_id=%d, mask=*0x%x)", id, class_id, mask);

	if (class_id != 0 && class_id != 2)
	{
		return CELL_EINVAL;
	}

	const auto t = Emu.GetCPU().GetRawSPUThread(id);

	if (!t)
	{
		return CELL_ESRCH;
	}

	auto& spu = static_cast<RawSPUThread&>(*t);

	*mask = (class_id ? spu.int2 : spu.int0).mask.load();

	return CELL_OK;
}

s32 sys_raw_spu_set_int_stat(u32 id, u32 class_id, u64 stat)
{
	sys_spu.Log("sys_raw_spu_set_int_stat(id=%d, class_id=%d, stat=0x%llx)", id, class_id, stat);

	if (class_id != 0 && class_id != 2)
	{
		return CELL_EINVAL;
	}

	const auto t = Emu.GetCPU().GetRawSPUThread(id);

	if (!t)
	{
		return CELL_ESRCH;
	}

	auto& spu = static_cast<RawSPUThread&>(*t);

	(class_id ? spu.int2 : spu.int0).clear(stat);

	return CELL_OK;
}

s32 sys_raw_spu_get_int_stat(u32 id, u32 class_id, vm::ptr<u64> stat)
{
	sys_spu.Log("sys_raw_spu_get_int_stat(id=%d, class_id=%d, stat=*0x%x)", id, class_id, stat);

	if (class_id != 0 && class_id != 2)
	{
		return CELL_EINVAL;
	}

	const auto t = Emu.GetCPU().GetRawSPUThread(id);

	if (!t)
	{
		return CELL_ESRCH;
	}

	auto& spu = static_cast<RawSPUThread&>(*t);

	*stat = (class_id ? spu.int2 : spu.int0).stat.load();

	return CELL_OK;
}

s32 sys_raw_spu_read_puint_mb(u32 id, vm::ptr<u32> value)
{
	sys_spu.Log("sys_raw_spu_read_puint_mb(id=%d, value=*0x%x)", id, value);

	const auto t = Emu.GetCPU().GetRawSPUThread(id);

	if (!t)
	{
		return CELL_ESRCH;
	}

	auto& spu = static_cast<RawSPUThread&>(*t);

	*value = spu.ch_out_intr_mbox.pop_uncond();

	return CELL_OK;
}

s32 sys_raw_spu_set_spu_cfg(u32 id, u32 value)
{
	sys_spu.Log("sys_raw_spu_set_spu_cfg(id=%d, value=0x%x)", id, value);

	if (value > 3)
	{
		sys_spu.Fatal("sys_raw_spu_set_spu_cfg(id=%d, value=0x%x)", id, value);
	}

	const auto t = Emu.GetCPU().GetRawSPUThread(id);

	if (!t)
	{
		return CELL_ESRCH;
	}

	auto& spu = static_cast<RawSPUThread&>(*t);

	spu.snr_config = value;

	return CELL_OK;
}

s32 sys_raw_spu_get_spu_cfg(u32 id, vm::ptr<u32> value)
{
	sys_spu.Log("sys_raw_spu_get_spu_afg(id=%d, value=*0x%x)", id, value);

	const auto t = Emu.GetCPU().GetRawSPUThread(id);

	if (!t)
	{
		return CELL_ESRCH;
	}

	auto& spu = static_cast<RawSPUThread&>(*t);

	*value = (u32)spu.snr_config;

	return CELL_OK;
}

void sys_spu_thread_exit(SPUThread & spu, s32 status)
{
	// Cancel any pending status update requests
	spu.set_ch_value(MFC_WrTagUpdate, 0);
	while (spu.get_ch_count(MFC_RdTagStat) != 1);
	spu.get_ch_value(MFC_RdTagStat);

	// Wait for all pending DMA operations to complete
	spu.set_ch_value(MFC_WrTagMask, 0xFFFFFFFF);
	spu.set_ch_value(MFC_WrTagUpdate, MFC_TAG_UPDATE_ALL);
	spu.get_ch_value(MFC_RdTagStat);

	spu.set_ch_value(SPU_WrOutMbox, status);
	spu.stop_and_signal(0x102);
}

void sys_spu_thread_group_exit(SPUThread & spu, s32 status)
{
	// Cancel any pending status update requests
	spu.set_ch_value(MFC_WrTagUpdate, 0);
	while (spu.get_ch_count(MFC_RdTagStat) != 1);
	spu.get_ch_value(MFC_RdTagStat);

	// Wait for all pending DMA operations to complete
	spu.set_ch_value(MFC_WrTagMask, 0xFFFFFFFF);
	spu.set_ch_value(MFC_WrTagUpdate, MFC_TAG_UPDATE_ALL);
	spu.get_ch_value(MFC_RdTagStat);

	spu.set_ch_value(SPU_WrOutMbox, status);
	spu.stop_and_signal(0x101);
}

s32 sys_spu_thread_send_event(SPUThread & spu, u8 spup, u32 data0, u32 data1)
{
	if (spup > 0x3F)
	{
		return CELL_EINVAL;
	}

	if (spu.get_ch_count(SPU_RdInMbox))
	{
		return CELL_EBUSY;
	}

	spu.set_ch_value(SPU_WrOutMbox, data1);
	spu.set_ch_value(SPU_WrOutIntrMbox, (spup << 24) | (data0 & 0x00FFFFFF));

	return spu.get_ch_value(SPU_RdInMbox);
}

s32 sys_spu_thread_switch_system_module(SPUThread & spu, u32 status)
{
	if (spu.get_ch_count(SPU_RdInMbox))
	{
		return CELL_EBUSY;
	}

	// Cancel any pending status update requests
	spu.set_ch_value(MFC_WrTagUpdate, 0);
	while (spu.get_ch_count(MFC_RdTagStat) != 1);
	spu.get_ch_value(MFC_RdTagStat);

	// Wait for all pending DMA operations to complete
	spu.set_ch_value(MFC_WrTagMask, 0xFFFFFFFF);
	spu.set_ch_value(MFC_WrTagUpdate, MFC_TAG_UPDATE_ALL);
	spu.get_ch_value(MFC_RdTagStat);

	s32 result;

	do
	{
		spu.set_ch_value(SPU_WrOutMbox, status);
		spu.stop_and_signal(0x120);
	}
	while ((result = spu.get_ch_value(SPU_RdInMbox)) == CELL_EBUSY);

	return result;
}
