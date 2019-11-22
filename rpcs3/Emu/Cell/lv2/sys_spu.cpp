#include "stdafx.h"
#include "sys_spu.h"

#include "Emu/System.h"
#include "Emu/IdManager.h"
#include "Crypto/unself.h"
#include "Crypto/unedat.h"
#include "Crypto/sha1.h"
#include "Loader/ELF.h"
#include "Utilities/bin_patch.h"

#include "Emu/Cell/ErrorCodes.h"
#include "Emu/Cell/PPUThread.h"
#include "Emu/Cell/RawSPUThread.h"
#include "sys_interrupt.h"
#include "sys_mmapper.h"
#include "sys_event.h"

LOG_CHANNEL(sys_spu);

void sys_spu_image::load(const fs::file& stream)
{
	const spu_exec_object obj{stream, 0, elf_opt::no_sections + elf_opt::no_data};

	if (obj != elf_error::ok)
	{
		fmt::throw_exception("Failed to load SPU image: %s" HERE, obj.get_error());
	}

	for (const auto& shdr : obj.shdrs)
	{
		LOG_NOTICE(SPU, "** Section: sh_type=0x%x, addr=0x%llx, size=0x%llx, flags=0x%x", shdr.sh_type, shdr.sh_addr, shdr.sh_size, shdr.sh_flags);
	}

	for (const auto& prog : obj.progs)
	{
		LOG_NOTICE(SPU, "** Segment: p_type=0x%x, p_vaddr=0x%llx, p_filesz=0x%llx, p_memsz=0x%llx, flags=0x%x", prog.p_type, prog.p_vaddr, prog.p_filesz, prog.p_memsz, prog.p_flags);

		if (prog.p_type != SYS_SPU_SEGMENT_TYPE_COPY && prog.p_type != SYS_SPU_SEGMENT_TYPE_INFO)
		{
			LOG_ERROR(SPU, "Unknown program type (0x%x)", prog.p_type);
		}
	}

	type        = SYS_SPU_IMAGE_TYPE_KERNEL;

	nsegs       = sys_spu_image::get_nsegs(obj.progs);

	const u32 mem_size = nsegs * sizeof(sys_spu_segment) + ::size32(stream);
	segs        = vm::cast(vm::alloc(mem_size, vm::main));

	// Write ID and save entry
	entry_point = idm::make<lv2_obj, lv2_spu_image>(+obj.header.e_entry);

	const u32 src = segs.addr() + nsegs * sizeof(sys_spu_segment);

	stream.seek(0);
	stream.read(vm::base(src), stream.size());

	if (nsegs < 0 || sys_spu_image::fill(segs, nsegs, obj.progs, src) != nsegs)
	{
		fmt::throw_exception("Failed to load SPU segments (%d)" HERE, nsegs);
	}

	vm::page_protect(segs.addr(), ::align(mem_size, 4096), 0, 0, vm::page_writable);
}

void sys_spu_image::free()
{
	if (type == SYS_SPU_IMAGE_TYPE_KERNEL)
	{
		vm::dealloc_verbose_nothrow(segs.addr(), vm::main);
	}
}

void sys_spu_image::deploy(u32 loc, sys_spu_segment* segs, u32 nsegs)
{
	// Segment info dump
	std::string dump;

	// Executable hash
	sha1_context sha;
	sha1_starts(&sha);
	u8 sha1_hash[20];

	for (u32 i = 0; i < nsegs; i++)
	{
		auto& seg = segs[i];

		fmt::append(dump, "\n\t[%d] t=0x%x, ls=0x%x, size=0x%x, addr=0x%x", i, seg.type, seg.ls, seg.size, seg.addr);

		sha1_update(&sha, (uchar*)&seg.type, sizeof(seg.type));

		// Hash big-endian values
		if (seg.type == SYS_SPU_SEGMENT_TYPE_COPY)
		{
			std::memcpy(vm::base(loc + seg.ls), vm::base(seg.addr), seg.size);
			sha1_update(&sha, (uchar*)&seg.size, sizeof(seg.size));
			sha1_update(&sha, (uchar*)&seg.ls, sizeof(seg.ls));
			sha1_update(&sha, vm::_ptr<uchar>(seg.addr), seg.size);
		}
		else if (seg.type == SYS_SPU_SEGMENT_TYPE_FILL)
		{
			if ((seg.ls | seg.size) % 4)
			{
				LOG_ERROR(SPU, "Unaligned SPU FILL type segment (ls=0x%x, size=0x%x)", seg.ls, seg.size);
			}

			std::fill_n(vm::_ptr<u32>(loc + seg.ls), seg.size / 4, seg.addr);
			sha1_update(&sha, (uchar*)&seg.size, sizeof(seg.size));
			sha1_update(&sha, (uchar*)&seg.ls, sizeof(seg.ls));
			sha1_update(&sha, (uchar*)&seg.addr, sizeof(seg.addr));
		}
		else if (seg.type == SYS_SPU_SEGMENT_TYPE_INFO)
		{
			const be_t<u32> size = seg.size + 0x14; // Workaround
			sha1_update(&sha, (uchar*)&size, sizeof(size));
		}
	}

	sha1_finish(&sha, sha1_hash);

	// Format patch name
	std::string hash("SPU-0000000000000000000000000000000000000000");
	for (u32 i = 0; i < sizeof(sha1_hash); i++)
	{
		constexpr auto pal = "0123456789abcdef";
		hash[4 + i * 2] = pal[sha1_hash[i] >> 4];
		hash[5 + i * 2] = pal[sha1_hash[i] & 15];
	}

	// Apply the patch
	auto applied = g_fxo->get<patch_engine>()->apply(hash, vm::_ptr<u8>(loc));

	if (!Emu.GetTitleID().empty())
	{
		// Alternative patch
		applied += g_fxo->get<patch_engine>()->apply(Emu.GetTitleID() + '-' + hash, vm::_ptr<u8>(loc));
	}

	LOG_NOTICE(LOADER, "Loaded SPU image: %s (<- %u)%s", hash, applied, dump);
}

error_code sys_spu_initialize(ppu_thread& ppu, u32 max_usable_spu, u32 max_raw_spu)
{
	vm::temporary_unlock(ppu);

	sys_spu.warning("sys_spu_initialize(max_usable_spu=%d, max_raw_spu=%d)", max_usable_spu, max_raw_spu);

	if (max_raw_spu > 5)
	{
		return CELL_EINVAL;
	}

	return CELL_OK;
}

error_code _sys_spu_image_get_information(ppu_thread& ppu, vm::ptr<sys_spu_image> img, vm::ptr<u32> entry_point, vm::ptr<s32> nsegs)
{
	vm::temporary_unlock(ppu);

	sys_spu.warning("_sys_spu_image_get_information(img=*0x%x, entry_point=*0x%x, nsegs=*0x%x)", img, entry_point, nsegs);

	if (img->type != SYS_SPU_IMAGE_TYPE_KERNEL)
	{
		return CELL_EINVAL;
	}

	const auto image = idm::get<lv2_obj, lv2_spu_image>(img->entry_point);

	if (!image)
	{
		return CELL_ESRCH;
	}

	*entry_point = image->e_entry;
	*nsegs       = img->nsegs;
	return CELL_OK;
}

error_code sys_spu_image_open(ppu_thread& ppu, vm::ptr<sys_spu_image> img, vm::cptr<char> path)
{
	vm::temporary_unlock(ppu);

	sys_spu.warning("sys_spu_image_open(img=*0x%x, path=%s)", img, path);

	const fs::file elf_file = decrypt_self(fs::file(vfs::get(path.get_ptr())), g_fxo->get<loaded_npdrm_keys>()->devKlic.data());

	if (!elf_file)
	{
		sys_spu.error("sys_spu_image_open() error: failed to open %s!", path);
		return CELL_ENOENT;
	}

	img->load(elf_file);
	return CELL_OK;
}

error_code _sys_spu_image_import(ppu_thread& ppu, vm::ptr<sys_spu_image> img, u32 src, u32 size, u32 arg4)
{
	vm::temporary_unlock(ppu);

	sys_spu.warning("_sys_spu_image_import(img=*0x%x, src=*0x%x, size=0x%x, arg4=0x%x)", img, src, size, arg4);

	img->load(fs::file{vm::base(src), size});
	return CELL_OK;
}

error_code _sys_spu_image_close(ppu_thread& ppu, vm::ptr<sys_spu_image> img)
{
	vm::temporary_unlock(ppu);

	sys_spu.warning("_sys_spu_image_close(img=*0x%x)", img);

	if (img->type != SYS_SPU_IMAGE_TYPE_KERNEL)
	{
		return CELL_EINVAL;
	}

	if (!idm::remove<lv2_obj, lv2_spu_image>(img->entry_point))
	{
		return CELL_ESRCH;
	}

	vm::dealloc(img->segs.addr(), vm::main);
	return CELL_OK;
}

error_code _sys_spu_image_get_segments(ppu_thread& ppu, vm::ptr<sys_spu_image> img, vm::ptr<sys_spu_segment> segments, s32 nseg)
{
	vm::temporary_unlock(ppu);

	sys_spu.error("_sys_spu_image_get_segments(img=*0x%x, segments=*0x%x, nseg=%d)", img, segments, nseg);

	// TODO: apply SPU patches
	std::memcpy(segments.get_ptr(), img->segs.get_ptr(), sizeof(sys_spu_segment) * nseg);
	return CELL_OK;
}

error_code sys_spu_thread_initialize(ppu_thread& ppu, vm::ptr<u32> thread, u32 group_id, u32 spu_num, vm::ptr<sys_spu_image> img, vm::ptr<sys_spu_thread_attribute> attr, vm::ptr<sys_spu_thread_argument> arg)
{
	vm::temporary_unlock(ppu);

	sys_spu.warning("sys_spu_thread_initialize(thread=*0x%x, group=0x%x, spu_num=%d, img=*0x%x, attr=*0x%x, arg=*0x%x)", thread, group_id, spu_num, img, attr, arg);

	// Read thread name
	const std::string thread_name(attr->name.get_ptr(), attr->name ? attr->name_len - 1 : 0);

	const auto group = idm::get<lv2_spu_group>(group_id);

	if (!group)
	{
		return CELL_ESRCH;
	}

	std::lock_guard lock(group->mutex);

	if (spu_num >= group->threads.size())
	{
		return CELL_EINVAL;
	}

	if (img->type != SYS_SPU_IMAGE_TYPE_KERNEL && img->type != SYS_SPU_IMAGE_TYPE_USER)
	{
		return CELL_EINVAL;
	}

	if (group->threads[spu_num] || group->run_state != SPU_THREAD_GROUP_STATUS_NOT_INITIALIZED)
	{
		return CELL_EBUSY;
	}

	sys_spu_image image = *img;

	if (img->type == SYS_SPU_IMAGE_TYPE_KERNEL)
	{
		const auto handle = idm::get<lv2_obj, lv2_spu_image>(img->entry_point);

		if (!handle)
		{
			return CELL_ESRCH;
		}

		// Save actual entry point
		image.entry_point = handle->e_entry;
	}

	if (u32 option = attr->option)
	{
		sys_spu.todo("Unimplemented SPU Thread options (0x%x)", option);
	}

	const vm::addr_t ls_addr{verify("SPU LS" HERE, vm::alloc(0x80000, vm::main))};

	const u32 tid = idm::import<named_thread<spu_thread>>([&]()
	{
		const u32 tid = idm::last_id();

		std::string full_name = fmt::format("SPU[0x%x] Thread", tid);

		if (!thread_name.empty())
		{
			fmt::append(full_name, " (%s)", thread_name);
		}

		group->threads[spu_num] = std::make_shared<named_thread<spu_thread>>(full_name, ls_addr, group.get(), spu_num, thread_name);
		return group->threads[spu_num];
	});

	*thread = tid;

	group->args[spu_num] = {arg->arg1, arg->arg2, arg->arg3, arg->arg4};
	group->imgs[spu_num] = std::make_pair(image, std::vector<sys_spu_segment>());
	group->imgs[spu_num].second.assign(img->segs.get_ptr(), img->segs.get_ptr() + img->nsegs);

	if (++group->init == group->max_num)
	{
		if (g_cfg.core.max_spurs_threads < 6 && group->max_num > 0u + g_cfg.core.max_spurs_threads)
		{
			if (group->name.size() >= 20 && group->name.compare(group->name.size() - 20, 20, "CellSpursKernelGroup", 20) == 0)
			{
				// Hack: don't run more SPURS threads than specified.
				group->init = g_cfg.core.max_spurs_threads;

				LOG_SUCCESS(SPU, "HACK: '%s' (0x%x) limited to %u threads.", group->name, group_id, +g_cfg.core.max_spurs_threads);
			}
		}

		group->run_state = SPU_THREAD_GROUP_STATUS_INITIALIZED;
	}

	return CELL_OK;
}

error_code sys_spu_thread_set_argument(ppu_thread& ppu, u32 id, vm::ptr<sys_spu_thread_argument> arg)
{
	vm::temporary_unlock(ppu);

	sys_spu.warning("sys_spu_thread_set_argument(id=0x%x, arg=*0x%x)", id, arg);

	const auto thread = idm::get<named_thread<spu_thread>>(id);

	if (UNLIKELY(!thread || !thread->group))
	{
		return CELL_ESRCH;
	}

	const auto group = thread->group;

	std::lock_guard lock(group->mutex);

	group->args[thread->index] = {arg->arg1, arg->arg2, arg->arg3, arg->arg4};

	return CELL_OK;
}

error_code sys_spu_thread_get_exit_status(ppu_thread& ppu, u32 id, vm::ptr<u32> status)
{
	vm::temporary_unlock(ppu);

	sys_spu.warning("sys_spu_thread_get_exit_status(id=0x%x, status=*0x%x)", id, status);

	const auto thread = idm::get<named_thread<spu_thread>>(id);

	if (UNLIKELY(!thread || !thread->group))
	{
		return CELL_ESRCH;
	}

	if (thread->status & SPU_STATUS_STOPPED_BY_STOP)
	{
		*status = thread->ch_out_mbox.get_value();
		return CELL_OK;
	}

	return CELL_ESTAT;
}

error_code sys_spu_thread_group_create(ppu_thread& ppu, vm::ptr<u32> id, u32 num, s32 prio, vm::ptr<sys_spu_thread_group_attribute> attr)
{
	vm::temporary_unlock(ppu);

	sys_spu.warning("sys_spu_thread_group_create(id=*0x%x, num=%d, prio=%d, attr=*0x%x)", id, num, prio, attr);

	// TODO: max num value should be affected by sys_spu_initialize() settings

	if (attr->nsize > 0x80 || !num || num > 6 || ((prio < 16 || prio > 255) && (attr->type != SYS_SPU_THREAD_GROUP_TYPE_EXCLUSIVE_NON_CONTEXT && attr->type != SYS_SPU_THREAD_GROUP_TYPE_COOPERATE_WITH_SYSTEM)))
	{
		return CELL_EINVAL;
	}

	if (attr->type)
	{
		sys_spu.todo("Unsupported SPU Thread Group type (0x%x)", attr->type);
	}

	*id = idm::make<lv2_spu_group>(std::string(attr->name.get_ptr(), std::max<u32>(attr->nsize, 1) - 1), num, prio, attr->type, attr->ct);

	return CELL_OK;
}

error_code sys_spu_thread_group_destroy(ppu_thread& ppu, u32 id)
{
	vm::temporary_unlock(ppu);

	sys_spu.warning("sys_spu_thread_group_destroy(id=0x%x)", id);

	const auto group = idm::withdraw<lv2_spu_group>(id, [](lv2_spu_group& group) -> CellError
	{
		const auto _old = group.run_state.compare_and_swap(SPU_THREAD_GROUP_STATUS_INITIALIZED, SPU_THREAD_GROUP_STATUS_NOT_INITIALIZED);

		if (_old > SPU_THREAD_GROUP_STATUS_INITIALIZED)
		{
			return CELL_EBUSY;
		}

		return {};
	});

	if (!group)
	{
		return CELL_ESRCH;
	}

	if (group.ret)
	{
		return group.ret;
	}

	// Cleanup
	for (auto& ptr : group->threads)
	{
		if (auto thread = std::move(ptr))
		{
			idm::remove<named_thread<spu_thread>>(thread->id);
		}
	}

	return CELL_OK;
}

error_code sys_spu_thread_group_start(ppu_thread& ppu, u32 id)
{
	vm::temporary_unlock(ppu);

	sys_spu.trace("sys_spu_thread_group_start(id=0x%x)", id);

	const auto group = idm::get<lv2_spu_group>(id, [](lv2_spu_group& group)
	{
		// SPU_THREAD_GROUP_STATUS_READY state is not used
		return group.run_state.compare_and_swap_test(SPU_THREAD_GROUP_STATUS_INITIALIZED, SPU_THREAD_GROUP_STATUS_RUNNING);
	});

	if (!group)
	{
		return CELL_ESRCH;
	}

	if (!group.ret)
	{
		return CELL_ESTAT;
	}

	std::lock_guard lock(group->mutex);

	u32 max_threads = +group->init;

	group->join_state = 0;
	group->running = max_threads;
	u32 run_threads = max_threads;

	for (auto& thread : group->threads)
	{
		if (!run_threads)
		{
			break;
		}

		if (thread && run_threads--)
		{
			auto& args = group->args[thread->index];
			auto& img = group->imgs[thread->index];

			sys_spu_image::deploy(thread->offset, img.second.data(), img.first.nsegs);

			thread->cpu_init();
			thread->npc = img.first.entry_point;
			thread->gpr[3] = v128::from64(0, args[0]);
			thread->gpr[4] = v128::from64(0, args[1]);
			thread->gpr[5] = v128::from64(0, args[2]);
			thread->gpr[6] = v128::from64(0, args[3]);

			thread->status.exchange(SPU_STATUS_RUNNING);
		}
	}

	// Because SPU_THREAD_GROUP_STATUS_READY is not possible, run event is delivered immediately
	// TODO: check data2 and data3
	group->send_run_event(id, 0, 0);

	u32 ran_threads = max_threads;

	for (auto& thread : group->threads)
	{
		if (!ran_threads)
		{
			break;
		}

		if (thread && ran_threads--)
		{
			thread->state -= cpu_flag::stop;
			thread_ctrl::notify(*thread);
		}
	}

	return CELL_OK;
}

error_code sys_spu_thread_group_suspend(ppu_thread& ppu, u32 id)
{
	vm::temporary_unlock(ppu);

	sys_spu.trace("sys_spu_thread_group_suspend(id=0x%x)", id);

	const auto group = idm::get<lv2_spu_group>(id);

	if (!group)
	{
		return CELL_ESRCH;
	}

	if (group->type & SYS_SPU_THREAD_GROUP_TYPE_EXCLUSIVE_NON_CONTEXT) // this check may be inaccurate
	{
		return CELL_EINVAL;
	}

	std::lock_guard lock(group->mutex);

	if (group->run_state <= SPU_THREAD_GROUP_STATUS_INITIALIZED || group->run_state == SPU_THREAD_GROUP_STATUS_STOPPED)
	{
		return CELL_ESTAT;
	}

	// SPU_THREAD_GROUP_STATUS_READY state is not used

	if (group->run_state == SPU_THREAD_GROUP_STATUS_RUNNING)
	{
		group->run_state = SPU_THREAD_GROUP_STATUS_SUSPENDED;
	}
	else if (group->run_state == SPU_THREAD_GROUP_STATUS_WAITING)
	{
		group->run_state = SPU_THREAD_GROUP_STATUS_WAITING_AND_SUSPENDED;
	}
	else if (group->run_state == SPU_THREAD_GROUP_STATUS_SUSPENDED || group->run_state == SPU_THREAD_GROUP_STATUS_WAITING_AND_SUSPENDED)
	{
		return CELL_OK;
	}
	else
	{
		return CELL_ESTAT;
	}

	for (auto& thread : group->threads)
	{
		if (thread)
		{
			thread->state += cpu_flag::suspend;
		}
	}

	return CELL_OK;
}

error_code sys_spu_thread_group_resume(ppu_thread& ppu, u32 id)
{
	vm::temporary_unlock(ppu);

	sys_spu.trace("sys_spu_thread_group_resume(id=0x%x)", id);

	const auto group = idm::get<lv2_spu_group>(id);

	if (!group)
	{
		return CELL_ESRCH;
	}

	if (group->type & SYS_SPU_THREAD_GROUP_TYPE_EXCLUSIVE_NON_CONTEXT) // this check may be inaccurate
	{
		return CELL_EINVAL;
	}

	std::lock_guard lock(group->mutex);

	// SPU_THREAD_GROUP_STATUS_READY state is not used

	if (group->run_state == SPU_THREAD_GROUP_STATUS_SUSPENDED)
	{
		group->run_state = SPU_THREAD_GROUP_STATUS_RUNNING;
	}
	else if (group->run_state == SPU_THREAD_GROUP_STATUS_WAITING_AND_SUSPENDED)
	{
		group->run_state = SPU_THREAD_GROUP_STATUS_WAITING;
	}
	else
	{
		return CELL_ESTAT;
	}

	for (auto& thread : group->threads)
	{
		if (thread)
		{
			thread->state -= cpu_flag::suspend;
			thread_ctrl::notify(*thread);
		}
	}

	return CELL_OK;
}

error_code sys_spu_thread_group_yield(ppu_thread& ppu, u32 id)
{
	vm::temporary_unlock(ppu);

	sys_spu.trace("sys_spu_thread_group_yield(id=0x%x)", id);

	const auto group = idm::get<lv2_spu_group>(id);

	if (!group)
	{
		return CELL_ESRCH;
	}

	if (group->type & SYS_SPU_THREAD_GROUP_TYPE_EXCLUSIVE_NON_CONTEXT) // this check may be inaccurate
	{
		return CELL_OK;
	}

	if (group->run_state != SPU_THREAD_GROUP_STATUS_RUNNING)
	{
		return CELL_ESTAT;
	}

	// SPU_THREAD_GROUP_STATUS_READY state is not used, so this function does nothing

	return CELL_OK;
}

error_code sys_spu_thread_group_terminate(ppu_thread& ppu, u32 id, s32 value)
{
	vm::temporary_unlock(ppu);

	sys_spu.trace("sys_spu_thread_group_terminate(id=0x%x, value=0x%x)", id, value);

	// The id can be either SPU Thread Group or SPU Thread
	const auto thread = idm::get<named_thread<spu_thread>>(id);
	const auto _group = idm::get<lv2_spu_group>(id);
	const auto group = thread ? thread->group : _group.get();

	if (!group && (!thread || !thread->group))
	{
		return CELL_ESRCH;
	}

	if (thread)
	{
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

	std::unique_lock lock(group->mutex);

	if (group->run_state <= SPU_THREAD_GROUP_STATUS_INITIALIZED ||
		group->run_state == SPU_THREAD_GROUP_STATUS_WAITING ||
		group->run_state == SPU_THREAD_GROUP_STATUS_WAITING_AND_SUSPENDED)
	{
		return CELL_ESTAT;
	}

	for (auto& thread : group->threads)
	{
		if (thread)
		{
			thread->state += cpu_flag::stop;
		}
	}

	for (auto& thread : group->threads)
	{
		if (thread && group->running)
		{
			thread_ctrl::notify(*thread);
		}
	}

	group->exit_status = value;
	group->join_state = SYS_SPU_THREAD_GROUP_JOIN_TERMINATED;

	// Wait until the threads are actually stopped
	const u64 last_stop = group->stop_count - !group->running;

	while (group->stop_count == last_stop)
	{
		group->cond.wait(lock);
	}

	return CELL_OK;
}

error_code sys_spu_thread_group_join(ppu_thread& ppu, u32 id, vm::ptr<u32> cause, vm::ptr<u32> status)
{
	vm::temporary_unlock(ppu);

	sys_spu.trace("sys_spu_thread_group_join(id=0x%x, cause=*0x%x, status=*0x%x)", id, cause, status);

	const auto group = idm::get<lv2_spu_group>(id);

	if (!group)
	{
		return CELL_ESRCH;
	}

	do
	{
		std::unique_lock lock(group->mutex);

		if (group->run_state < SPU_THREAD_GROUP_STATUS_INITIALIZED)
		{
			return CELL_ESTAT;
		}

		if (group->waiter)
		{
			// another PPU thread is joining this thread group
			return CELL_EBUSY;
		}

		if (group->join_state && group->run_state == SPU_THREAD_GROUP_STATUS_INITIALIZED)
		{
			// Already signaled
			ppu.gpr[4] = group->join_state;
			ppu.gpr[5] = group->exit_status;
			group->join_state.release(0);
			break;
		}
		else
		{
			// Subscribe to receive status in r4-r5
			ppu.gpr[4] = 0;
			group->waiter = &ppu;
		}

		lv2_obj::sleep(ppu);

		while (!ppu.gpr[4])
		{
			if (ppu.is_stopped())
			{
				return 0;
			}

			group->cond.wait(lock);
		}
	}
	while (0);

	if (ppu.test_stopped())
	{
		return 0;
	}

	if (!cause)
	{
		return CELL_EFAULT;
	}

	*cause = static_cast<u32>(ppu.gpr[4]);

	if (!status)
	{
		return CELL_EFAULT;
	}

	*status = static_cast<s32>(ppu.gpr[5]);
	return CELL_OK;
}

error_code sys_spu_thread_group_set_priority(ppu_thread& ppu, u32 id, s32 priority)
{
	vm::temporary_unlock(ppu);

	sys_spu.trace("sys_spu_thread_group_set_priority(id=0x%x, priority=%d)", id, priority);

	if (priority < 16 || priority > 255)
	{
		return CELL_EINVAL;
	}

	const auto group = idm::get<lv2_spu_group>(id);

	if (!group)
	{
		return CELL_ESRCH;
	}

	if (group->type == SYS_SPU_THREAD_GROUP_TYPE_EXCLUSIVE_NON_CONTEXT)
	{
		return CELL_EINVAL;
	}

	group->prio = priority;

	return CELL_OK;
}

error_code sys_spu_thread_group_get_priority(ppu_thread& ppu, u32 id, vm::ptr<s32> priority)
{
	vm::temporary_unlock(ppu);

	sys_spu.trace("sys_spu_thread_group_get_priority(id=0x%x, priority=*0x%x)", id, priority);

	const auto group = idm::get<lv2_spu_group>(id);

	if (!group)
	{
		return CELL_ESRCH;
	}

	if (group->type == SYS_SPU_THREAD_GROUP_TYPE_EXCLUSIVE_NON_CONTEXT)
	{
		*priority = 0;
	}
	else
	{
		*priority = group->prio;
	}

	return CELL_OK;
}

error_code sys_spu_thread_write_ls(ppu_thread& ppu, u32 id, u32 lsa, u64 value, u32 type)
{
	vm::temporary_unlock(ppu);

	sys_spu.trace("sys_spu_thread_write_ls(id=0x%x, lsa=0x%05x, value=0x%llx, type=%d)", id, lsa, value, type);

	if (lsa >= 0x40000 || type > 8 || !type || (type | lsa) & (type - 1)) // check range and alignment
	{
		return CELL_EINVAL;
	}

	const auto thread = idm::get<named_thread<spu_thread>>(id);

	if (UNLIKELY(!thread || !thread->group))
	{
		return CELL_ESRCH;
	}

	const auto group = thread->group;

	std::lock_guard lock(group->mutex);

	if (group->run_state < SPU_THREAD_GROUP_STATUS_WAITING || group->run_state > SPU_THREAD_GROUP_STATUS_RUNNING)
	{
		return CELL_ESTAT;
	}

	switch (type)
	{
	case 1: thread->_ref<u8>(lsa) = (u8)value; break;
	case 2: thread->_ref<u16>(lsa) = (u16)value; break;
	case 4: thread->_ref<u32>(lsa) = (u32)value; break;
	case 8: thread->_ref<u64>(lsa) = value; break;
	default: ASSUME(0);
	}

	return CELL_OK;
}

error_code sys_spu_thread_read_ls(ppu_thread& ppu, u32 id, u32 lsa, vm::ptr<u64> value, u32 type)
{
	vm::temporary_unlock(ppu);

	sys_spu.trace("sys_spu_thread_read_ls(id=0x%x, lsa=0x%05x, value=*0x%x, type=%d)", id, lsa, value, type);

	if (lsa >= 0x40000 || type > 8 || !type || (type | lsa) & (type - 1)) // check range and alignment
	{
		return CELL_EINVAL;
	}

	const auto thread = idm::get<named_thread<spu_thread>>(id);

	if (UNLIKELY(!thread || !thread->group))
	{
		return CELL_ESRCH;
	}

	const auto group = thread->group;

	std::lock_guard lock(group->mutex);

	if (group->run_state < SPU_THREAD_GROUP_STATUS_WAITING || group->run_state > SPU_THREAD_GROUP_STATUS_RUNNING)
	{
		return CELL_ESTAT;
	}

	switch (type)
	{
	case 1: *value = thread->_ref<u8>(lsa); break;
	case 2: *value = thread->_ref<u16>(lsa); break;
	case 4: *value = thread->_ref<u32>(lsa); break;
	case 8: *value = thread->_ref<u64>(lsa); break;
	default: ASSUME(0);
	}

	return CELL_OK;
}

error_code sys_spu_thread_write_spu_mb(ppu_thread& ppu, u32 id, u32 value)
{
	vm::temporary_unlock(ppu);

	sys_spu.warning("sys_spu_thread_write_spu_mb(id=0x%x, value=0x%x)", id, value);

	const auto thread = idm::get<named_thread<spu_thread>>(id);

	if (UNLIKELY(!thread || !thread->group))
	{
		return CELL_ESRCH;
	}

	const auto group = thread->group;

	std::lock_guard lock(group->mutex);

	thread->ch_in_mbox.push(*thread, value);

	return CELL_OK;
}

error_code sys_spu_thread_set_spu_cfg(ppu_thread& ppu, u32 id, u64 value)
{
	vm::temporary_unlock(ppu);

	sys_spu.warning("sys_spu_thread_set_spu_cfg(id=0x%x, value=0x%x)", id, value);

	if (value > 3)
	{
		return CELL_EINVAL;
	}

	const auto thread = idm::get<named_thread<spu_thread>>(id);

	if (UNLIKELY(!thread || !thread->group))
	{
		return CELL_ESRCH;
	}

	thread->snr_config = value;

	return CELL_OK;
}

error_code sys_spu_thread_get_spu_cfg(ppu_thread& ppu, u32 id, vm::ptr<u64> value)
{
	vm::temporary_unlock(ppu);

	sys_spu.warning("sys_spu_thread_get_spu_cfg(id=0x%x, value=*0x%x)", id, value);

	const auto thread = idm::get<named_thread<spu_thread>>(id);

	if (UNLIKELY(!thread || !thread->group))
	{
		return CELL_ESRCH;
	}

	*value = thread->snr_config;

	return CELL_OK;
}

error_code sys_spu_thread_write_snr(ppu_thread& ppu, u32 id, u32 number, u32 value)
{
	vm::temporary_unlock(ppu);

	sys_spu.trace("sys_spu_thread_write_snr(id=0x%x, number=%d, value=0x%x)", id, number, value);

	const auto thread = idm::get<named_thread<spu_thread>>(id);

	if (UNLIKELY(!thread || !thread->group))
	{
		return CELL_ESRCH;
	}

	if (number > 1)
	{
		return CELL_EINVAL;
	}

	thread->push_snr(number, value);

	return CELL_OK;
}

error_code sys_spu_thread_group_connect_event(ppu_thread& ppu, u32 id, u32 eq, u32 et)
{
	vm::temporary_unlock(ppu);

	sys_spu.warning("sys_spu_thread_group_connect_event(id=0x%x, eq=0x%x, et=%d)", id, eq, et);

	const auto group = idm::get<lv2_spu_group>(id);
	const auto queue = idm::get<lv2_obj, lv2_event_queue>(eq);

	if (!group || !queue)
	{
		return CELL_ESRCH;
	}

	std::lock_guard lock(group->mutex);

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
		sys_spu.error("sys_spu_thread_group_connect_event(): unknown event type (%d)", et);
		return CELL_EINVAL;
	}
	}

	return CELL_OK;
}

error_code sys_spu_thread_group_disconnect_event(ppu_thread& ppu, u32 id, u32 et)
{
	vm::temporary_unlock(ppu);

	sys_spu.warning("sys_spu_thread_group_disconnect_event(id=0x%x, et=%d)", id, et);

	const auto group = idm::get<lv2_spu_group>(id);

	if (!group)
	{
		return CELL_ESRCH;
	}

	std::lock_guard lock(group->mutex);

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
		sys_spu.error("sys_spu_thread_group_disconnect_event(): unknown event type (%d)", et);
		return CELL_EINVAL;
	}
	}

	return CELL_OK;
}

error_code sys_spu_thread_connect_event(ppu_thread& ppu, u32 id, u32 eq, u32 et, u8 spup)
{
	vm::temporary_unlock(ppu);

	sys_spu.warning("sys_spu_thread_connect_event(id=0x%x, eq=0x%x, et=%d, spup=%d)", id, eq, et, spup);

	const auto thread = idm::get<named_thread<spu_thread>>(id);
	const auto queue = idm::get<lv2_obj, lv2_event_queue>(eq);

	if (UNLIKELY(!queue || !thread || !thread->group))
	{
		return CELL_ESRCH;
	}

	if (et != SYS_SPU_THREAD_EVENT_USER || spup > 63)
	{
		sys_spu.error("sys_spu_thread_connect_event(): invalid arguments (et=%d, spup=%d, queue->type=%d)", et, spup, queue->type);
		return CELL_EINVAL;
	}

	std::lock_guard lock(thread->group->mutex);

	auto& port = thread->spup[spup];

	if (!port.expired())
	{
		return CELL_EISCONN;
	}

	port = queue;

	return CELL_OK;
}

error_code sys_spu_thread_disconnect_event(ppu_thread& ppu, u32 id, u32 et, u8 spup)
{
	vm::temporary_unlock(ppu);

	sys_spu.warning("sys_spu_thread_disconnect_event(id=0x%x, et=%d, spup=%d)", id, et, spup);

	const auto thread = idm::get<named_thread<spu_thread>>(id);

	if (UNLIKELY(!thread || !thread->group))
	{
		return CELL_ESRCH;
	}

	if (et != SYS_SPU_THREAD_EVENT_USER || spup > 63)
	{
		sys_spu.error("sys_spu_thread_disconnect_event(): invalid arguments (et=%d, spup=%d)", et, spup);
		return CELL_EINVAL;
	}

	std::lock_guard lock(thread->group->mutex);

	auto& port = thread->spup[spup];

	if (port.expired())
	{
		return CELL_ENOTCONN;
	}

	port.reset();

	return CELL_OK;
}

error_code sys_spu_thread_bind_queue(ppu_thread& ppu, u32 id, u32 spuq, u32 spuq_num)
{
	vm::temporary_unlock(ppu);

	sys_spu.warning("sys_spu_thread_bind_queue(id=0x%x, spuq=0x%x, spuq_num=0x%x)", id, spuq, spuq_num);

	const auto thread = idm::get<named_thread<spu_thread>>(id);
	const auto queue = idm::get<lv2_obj, lv2_event_queue>(spuq);

	if (UNLIKELY(!queue || !thread || !thread->group))
	{
		return CELL_ESRCH;
	}

	if (queue->type != SYS_SPU_QUEUE)
	{
		return CELL_EINVAL;
	}

	std::lock_guard lock(thread->group->mutex);

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

error_code sys_spu_thread_unbind_queue(ppu_thread& ppu, u32 id, u32 spuq_num)
{
	vm::temporary_unlock(ppu);

	sys_spu.warning("sys_spu_thread_unbind_queue(id=0x%x, spuq_num=0x%x)", id, spuq_num);

	const auto thread = idm::get<named_thread<spu_thread>>(id);

	if (UNLIKELY(!thread || !thread->group))
	{
		return CELL_ESRCH;
	}

	std::lock_guard lock(thread->group->mutex);

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

error_code sys_spu_thread_group_connect_event_all_threads(ppu_thread& ppu, u32 id, u32 eq, u64 req, vm::ptr<u8> spup)
{
	vm::temporary_unlock(ppu);

	sys_spu.warning("sys_spu_thread_group_connect_event_all_threads(id=0x%x, eq=0x%x, req=0x%llx, spup=*0x%x)", id, eq, req, spup);

	const auto group = idm::get<lv2_spu_group>(id);
	const auto queue = idm::get<lv2_obj, lv2_event_queue>(eq);

	if (!group || !queue)
	{
		return CELL_ESRCH;
	}

	if (!req)
	{
		return CELL_EINVAL;
	}

	std::lock_guard lock(group->mutex);

	if (group->run_state < SPU_THREAD_GROUP_STATUS_INITIALIZED)
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
				if (!t->spup[port].expired())
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
			t->spup[port] = queue;
		}
	}

	*spup = port;

	return CELL_OK;
}

error_code sys_spu_thread_group_disconnect_event_all_threads(ppu_thread& ppu, u32 id, u8 spup)
{
	vm::temporary_unlock(ppu);

	sys_spu.warning("sys_spu_thread_group_disconnect_event_all_threads(id=0x%x, spup=%d)", id, spup);

	const auto group = idm::get<lv2_spu_group>(id);

	if (!group)
	{
		return CELL_ESRCH;
	}

	if (spup > 63)
	{
		return CELL_EINVAL;
	}

	std::lock_guard lock(group->mutex);

	for (auto& t : group->threads)
	{
		if (t)
		{
			t->spup[spup].reset();
		}
	}

	return CELL_OK;
}

error_code sys_spu_thread_group_log(ppu_thread& ppu, s32 command, vm::ptr<s32> stat)
{
	vm::temporary_unlock(ppu);

	sys_spu.warning("sys_spu_thread_group_log(command=0x%x, stat=*0x%x)", command, stat);

	struct spu_group_log_state_t
	{
		atomic_t<s32> state = SYS_SPU_THREAD_GROUP_LOG_ON;
	};

	const auto state = g_fxo->get<spu_group_log_state_t>();

	switch (command)
	{
	case SYS_SPU_THREAD_GROUP_LOG_GET_STATUS:
	{
		if (!stat)
		{
			return CELL_EFAULT;
		}

		*stat = state->state;
		break;
	}
	case SYS_SPU_THREAD_GROUP_LOG_ON:
	case SYS_SPU_THREAD_GROUP_LOG_OFF:
	{
		state->state.release(command);
		break;
	}
	default: return CELL_EINVAL;
	}

	return CELL_OK;
}

error_code sys_spu_thread_recover_page_fault(ppu_thread& ppu, u32 id)
{
	vm::temporary_unlock(ppu);

	sys_spu.warning("sys_spu_thread_recover_page_fault(id=0x%x)", id);

	const auto thread = idm::get<named_thread<spu_thread>>(id);

	if (UNLIKELY(!thread || !thread->group))
	{
		return CELL_ESRCH;
	}

	return mmapper_thread_recover_page_fault(id);
}

error_code sys_raw_spu_recover_page_fault(ppu_thread& ppu, u32 id)
{
	vm::temporary_unlock(ppu);

	sys_spu.warning("sys_raw_spu_recover_page_fault(id=0x%x)", id);

	const auto thread = idm::get<named_thread<spu_thread>>(spu_thread::find_raw_spu(id));

	if (UNLIKELY(!thread || thread->group))
	{
		return CELL_ESRCH;
	}

	return mmapper_thread_recover_page_fault(id);
}

error_code sys_raw_spu_create(ppu_thread& ppu, vm::ptr<u32> id, vm::ptr<void> attr)
{
	vm::temporary_unlock(ppu);

	sys_spu.warning("sys_raw_spu_create(id=*0x%x, attr=*0x%x)", id, attr);

	// TODO: check number set by sys_spu_initialize()

	if (!spu_thread::g_raw_spu_ctr.try_inc(5))
	{
		return CELL_EAGAIN;
	}

	u32 index = 0;

	// Find free RawSPU ID
	while (!spu_thread::g_raw_spu_id[index].try_inc(1))
	{
		if (++index == 5)
			index = 0;
	}

	const vm::addr_t ls_addr{verify(HERE, vm::falloc(RAW_SPU_BASE_ADDR + RAW_SPU_OFFSET * index, 0x40000, vm::spu))};

	const u32 tid = idm::make<named_thread<spu_thread>>(fmt::format("RawSPU[0x%x] Thread", index), ls_addr, nullptr, index, "");

	spu_thread::g_raw_spu_id[index] = verify("RawSPU ID" HERE, tid);

	*id = index;

	return CELL_OK;
}

error_code sys_raw_spu_destroy(ppu_thread& ppu, u32 id)
{
	vm::temporary_unlock(ppu);

	sys_spu.warning("sys_raw_spu_destroy(id=%d)", id);

	const auto thread = idm::get<named_thread<spu_thread>>(spu_thread::find_raw_spu(id));

	if (UNLIKELY(!thread || thread->group))
	{
		return CELL_ESRCH;
	}

	// TODO: CELL_EBUSY is not returned

	// Stop thread
	thread->state += cpu_flag::stop;

	// Kernel objects which must be removed
	std::unordered_map<lv2_obj*, u32, pointer_hash<lv2_obj, alignof(void*)>> to_remove;

	// Clear interrupt handlers
	for (auto& intr : thread->int_ctrl)
	{
		if (intr.tag)
		{
			if (auto handler = intr.tag->handler.lock())
			{
				// SLEEP
				handler->join();
				to_remove.emplace(handler.get(), 0);
			}

			to_remove.emplace(intr.tag.get(), 0);
		}
	}

	// Scan all kernel objects to determine IDs
	idm::select<lv2_obj>([&](u32 id, lv2_obj& obj)
	{
		const auto found = to_remove.find(&obj);

		if (found != to_remove.end())
		{
			found->second = id;
		}
	});

	// Remove IDs
	for (auto&& pair : to_remove)
	{
		if (pair.second >> 24 == 0xa)
			idm::remove<lv2_obj, lv2_int_tag>(pair.second);
		if (pair.second >> 24 == 0xb)
			idm::remove<lv2_obj, lv2_int_serv>(pair.second);
	}

	idm::remove<named_thread<spu_thread>>(thread->id);
	return CELL_OK;
}

error_code sys_raw_spu_create_interrupt_tag(ppu_thread& ppu, u32 id, u32 class_id, u32 hwthread, vm::ptr<u32> intrtag)
{
	vm::temporary_unlock(ppu);

	sys_spu.warning("sys_raw_spu_create_interrupt_tag(id=%d, class_id=%d, hwthread=0x%x, intrtag=*0x%x)", id, class_id, hwthread, intrtag);

	if (class_id != 0 && class_id != 2)
	{
		return CELL_EINVAL;
	}

	CellError error = {};

	const auto tag = idm::import<lv2_obj, lv2_int_tag>([&]()
	{
		std::shared_ptr<lv2_int_tag> result;

		auto thread = idm::check_unlocked<named_thread<spu_thread>>(spu_thread::find_raw_spu(id));

		if (!thread || thread->group)
		{
			error = CELL_ESRCH;
			return result;
		}

		auto& int_ctrl = thread->int_ctrl[class_id];

		if (int_ctrl.tag)
		{
			error = CELL_EAGAIN;
			return result;
		}

		result = std::make_shared<lv2_int_tag>();
		int_ctrl.tag = result;
		return result;
	});

	if (tag)
	{
		*intrtag = tag;
		return CELL_OK;
	}

	return error;
}

error_code sys_raw_spu_set_int_mask(ppu_thread& ppu, u32 id, u32 class_id, u64 mask)
{
	vm::temporary_unlock(ppu);

	sys_spu.trace("sys_raw_spu_set_int_mask(id=%d, class_id=%d, mask=0x%llx)", id, class_id, mask);

	if (class_id != 0 && class_id != 2)
	{
		return CELL_EINVAL;
	}

	const auto thread = idm::get<named_thread<spu_thread>>(spu_thread::find_raw_spu(id));

	if (UNLIKELY(!thread || thread->group))
	{
		return CELL_ESRCH;
	}

	thread->int_ctrl[class_id].mask.exchange(mask);

	return CELL_OK;
}

error_code sys_raw_spu_get_int_mask(ppu_thread& ppu, u32 id, u32 class_id, vm::ptr<u64> mask)
{
	vm::temporary_unlock(ppu);

	sys_spu.trace("sys_raw_spu_get_int_mask(id=%d, class_id=%d, mask=*0x%x)", id, class_id, mask);

	if (class_id != 0 && class_id != 2)
	{
		return CELL_EINVAL;
	}

	const auto thread = idm::get<named_thread<spu_thread>>(spu_thread::find_raw_spu(id));

	if (UNLIKELY(!thread || thread->group))
	{
		return CELL_ESRCH;
	}

	*mask = thread->int_ctrl[class_id].mask;

	return CELL_OK;
}

error_code sys_raw_spu_set_int_stat(ppu_thread& ppu, u32 id, u32 class_id, u64 stat)
{
	vm::temporary_unlock(ppu);

	sys_spu.trace("sys_raw_spu_set_int_stat(id=%d, class_id=%d, stat=0x%llx)", id, class_id, stat);

	if (class_id != 0 && class_id != 2)
	{
		return CELL_EINVAL;
	}

	const auto thread = idm::get<named_thread<spu_thread>>(spu_thread::find_raw_spu(id));

	if (UNLIKELY(!thread || thread->group))
	{
		return CELL_ESRCH;
	}

	thread->int_ctrl[class_id].clear(stat);

	return CELL_OK;
}

error_code sys_raw_spu_get_int_stat(ppu_thread& ppu, u32 id, u32 class_id, vm::ptr<u64> stat)
{
	vm::temporary_unlock(ppu);

	sys_spu.trace("sys_raw_spu_get_int_stat(id=%d, class_id=%d, stat=*0x%x)", id, class_id, stat);

	if (class_id != 0 && class_id != 2)
	{
		return CELL_EINVAL;
	}

	const auto thread = idm::get<named_thread<spu_thread>>(spu_thread::find_raw_spu(id));

	if (UNLIKELY(!thread || thread->group))
	{
		return CELL_ESRCH;
	}

	*stat = thread->int_ctrl[class_id].stat;

	return CELL_OK;
}

error_code sys_raw_spu_read_puint_mb(ppu_thread& ppu, u32 id, vm::ptr<u32> value)
{
	vm::temporary_unlock(ppu);

	sys_spu.trace("sys_raw_spu_read_puint_mb(id=%d, value=*0x%x)", id, value);

	const auto thread = idm::get<named_thread<spu_thread>>(spu_thread::find_raw_spu(id));

	if (UNLIKELY(!thread || thread->group))
	{
		return CELL_ESRCH;
	}

	*value = thread->ch_out_intr_mbox.pop(*thread);

	return CELL_OK;
}

error_code sys_raw_spu_set_spu_cfg(ppu_thread& ppu, u32 id, u32 value)
{
	vm::temporary_unlock(ppu);

	sys_spu.trace("sys_raw_spu_set_spu_cfg(id=%d, value=0x%x)", id, value);

	if (value > 3)
	{
		fmt::throw_exception("Unexpected value (0x%x)" HERE, value);
	}

	const auto thread = idm::get<named_thread<spu_thread>>(spu_thread::find_raw_spu(id));

	if (UNLIKELY(!thread || thread->group))
	{
		return CELL_ESRCH;
	}

	thread->snr_config = value;

	return CELL_OK;
}

error_code sys_raw_spu_get_spu_cfg(ppu_thread& ppu, u32 id, vm::ptr<u32> value)
{
	vm::temporary_unlock(ppu);

	sys_spu.trace("sys_raw_spu_get_spu_afg(id=%d, value=*0x%x)", id, value);

	const auto thread = idm::get<named_thread<spu_thread>>(spu_thread::find_raw_spu(id));

	if (UNLIKELY(!thread || thread->group))
	{
		return CELL_ESRCH;
	}

	*value = (u32)thread->snr_config;

	return CELL_OK;
}
