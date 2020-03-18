#include "stdafx.h"
#include "sys_spu.h"

#include "Emu/System.h"
#include "Emu/VFS.h"
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
#include "sys_process.h"
#include "sys_memory.h"
#include "sys_mmapper.h"
#include "sys_event.h"
#include "sys_fs.h"

LOG_CHANNEL(sys_spu);

extern u64 get_timebased_time();

void sys_spu_image::load(const fs::file& stream)
{
	const spu_exec_object obj{stream, 0, elf_opt::no_sections + elf_opt::no_data};

	if (obj != elf_error::ok)
	{
		fmt::throw_exception("Failed to load SPU image: %s" HERE, obj.get_error());
	}

	for (const auto& shdr : obj.shdrs)
	{
		spu_log.notice("** Section: sh_type=0x%x, addr=0x%llx, size=0x%llx, flags=0x%x", shdr.sh_type, shdr.sh_addr, shdr.sh_size, shdr.sh_flags);
	}

	for (const auto& prog : obj.progs)
	{
		spu_log.notice("** Segment: p_type=0x%x, p_vaddr=0x%llx, p_filesz=0x%llx, p_memsz=0x%llx, flags=0x%x", prog.p_type, prog.p_vaddr, prog.p_filesz, prog.p_memsz, prog.p_flags);

		if (prog.p_type != u32{SYS_SPU_SEGMENT_TYPE_COPY} && prog.p_type != u32{SYS_SPU_SEGMENT_TYPE_INFO})
		{
			spu_log.error("Unknown program type (0x%x)", prog.p_type);
		}
	}

	this->type = SYS_SPU_IMAGE_TYPE_KERNEL;
	const s32 nsegs = sys_spu_image::get_nsegs(obj.progs);

	const u32 mem_size = nsegs * sizeof(sys_spu_segment) + ::size32(stream);
	const vm::ptr<sys_spu_segment> segs = vm::cast(vm::alloc(mem_size, vm::main));

	const u32 entry = obj.header.e_entry;

	const u32 src = (segs + nsegs).addr();

	stream.seek(0);
	stream.read(vm::base(src), stream.size());

	if (nsegs <= 0 || nsegs > 0x20 || sys_spu_image::fill(segs, nsegs, obj.progs, src) != nsegs)
	{
		fmt::throw_exception("Failed to load SPU segments (%d)" HERE, nsegs);
	}

	// Write ID and save entry
	this->entry_point = idm::make<lv2_obj, lv2_spu_image>(+obj.header.e_entry, segs, nsegs);

	// Unused and set to 0
	this->nsegs = 0;
	this->segs = vm::null;

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

		sha1_update(&sha, reinterpret_cast<uchar*>(&seg.type), sizeof(seg.type));

		// Hash big-endian values
		if (seg.type == SYS_SPU_SEGMENT_TYPE_COPY)
		{
			std::memcpy(vm::base(loc + seg.ls), vm::base(seg.addr), seg.size);
			sha1_update(&sha, reinterpret_cast<uchar*>(&seg.size), sizeof(seg.size));
			sha1_update(&sha, reinterpret_cast<uchar*>(&seg.ls), sizeof(seg.ls));
			sha1_update(&sha, vm::_ptr<uchar>(seg.addr), seg.size);
		}
		else if (seg.type == SYS_SPU_SEGMENT_TYPE_FILL)
		{
			if ((seg.ls | seg.size) % 4)
			{
				spu_log.error("Unaligned SPU FILL type segment (ls=0x%x, size=0x%x)", seg.ls, seg.size);
			}

			std::fill_n(vm::_ptr<u32>(loc + seg.ls), seg.size / 4, seg.addr);
			sha1_update(&sha, reinterpret_cast<uchar*>(&seg.size), sizeof(seg.size));
			sha1_update(&sha, reinterpret_cast<uchar*>(&seg.ls), sizeof(seg.ls));
			sha1_update(&sha, reinterpret_cast<uchar*>(&seg.addr), sizeof(seg.addr));
		}
		else if (seg.type == SYS_SPU_SEGMENT_TYPE_INFO)
		{
			const be_t<u32> size = seg.size + 0x14; // Workaround
			sha1_update(&sha, reinterpret_cast<const uchar*>(&size), sizeof(size));
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

	spu_log.notice("Loaded SPU image: %s (<- %u)%s", hash, applied, dump);
}

// Get spu thread ptr, returns group ptr as well for refcounting
std::pair<named_thread<spu_thread>*, std::shared_ptr<lv2_spu_group>> lv2_spu_group::get_thread(u32 id)
{
	if (id >= 0x06000000)
	{
		// thread index is out of range (5 max)
		return {};
	}

	// Bits 0-23 contain group id (without id base)
	decltype(get_thread(0)) res{nullptr, idm::get<lv2_spu_group>((id & 0xFFFFFF) | (lv2_spu_group::id_base & ~0xFFFFFF))};

	// Bits 24-31 contain thread index within the group
	const u32 index = id >> 24;

	if (auto group = res.second.get(); group && group->init > index)
	{
		res.first = group->threads[index].get();
	}

	return res;
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
	*nsegs       = image->nsegs;
	return CELL_OK;
}

error_code sys_spu_image_open(ppu_thread& ppu, vm::ptr<sys_spu_image> img, vm::cptr<char> path)
{
	vm::temporary_unlock(ppu);

	sys_spu.warning("sys_spu_image_open(img=*0x%x, path=%s)", img, path);

	auto [fs_error, ppath, file] = lv2_file::open(path.get_ptr(), 0, 0);

	if (fs_error)
	{
		return {fs_error, path};
	}

	const fs::file elf_file = decrypt_self(std::move(file), g_fxo->get<loaded_npdrm_keys>()->devKlic.data());

	if (!elf_file)
	{
		sys_spu.error("sys_spu_image_open(): file %s is illegal for SPU image!", path);
		return {CELL_ENOEXEC, path};
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

	const auto handle = idm::withdraw<lv2_obj, lv2_spu_image>(img->entry_point);

	if (!handle)
	{
		return CELL_ESRCH;
	}

	verify(HERE), vm::dealloc(handle->segs.addr(), vm::main);
	return CELL_OK;
}

error_code _sys_spu_image_get_segments(ppu_thread& ppu, vm::ptr<sys_spu_image> img, vm::ptr<sys_spu_segment> segments, s32 nseg)
{
	vm::temporary_unlock(ppu);

	sys_spu.error("_sys_spu_image_get_segments(img=*0x%x, segments=*0x%x, nseg=%d)", img, segments, nseg);

	if (nseg <= 0 || nseg > 0x20 || img->type != SYS_SPU_IMAGE_TYPE_KERNEL)
	{
		return CELL_EINVAL;
	}

	const auto handle = idm::get<lv2_obj, lv2_spu_image>(img->entry_point);

	if (!handle)
	{
		return CELL_ESRCH;
	}

	// TODO: apply SPU patches
	std::memcpy(segments.get_ptr(), handle->segs.get_ptr(), sizeof(sys_spu_segment) * std::min<s32>(nseg, handle->nsegs));
	return CELL_OK;
}

error_code sys_spu_thread_initialize(ppu_thread& ppu, vm::ptr<u32> thread, u32 group_id, u32 spu_num, vm::ptr<sys_spu_image> img, vm::ptr<sys_spu_thread_attribute> attr, vm::ptr<sys_spu_thread_argument> arg)
{
	vm::temporary_unlock(ppu);

	sys_spu.warning("sys_spu_thread_initialize(thread=*0x%x, group=0x%x, spu_num=%d, img=*0x%x, attr=*0x%x, arg=*0x%x)", thread, group_id, spu_num, img, attr, arg);

	if (attr->name_len > 0x80 || attr->option & ~(SYS_SPU_THREAD_OPTION_DEC_SYNC_TB_ENABLE | SYS_SPU_THREAD_OPTION_ASYNC_INTR_ENABLE))
	{
		return CELL_EINVAL;
	}

	sys_spu_image image;

	switch (img->type)
	{
	case SYS_SPU_IMAGE_TYPE_KERNEL:
	{
		const auto handle = idm::get<lv2_obj, lv2_spu_image>(img->entry_point);

		if (!handle)
		{
			return CELL_ESRCH;
		}

		// Image information is stored in IDM
		image.entry_point = handle->e_entry;
		image.nsegs = handle->nsegs;
		image.segs = handle->segs;
		image.type = SYS_SPU_IMAGE_TYPE_KERNEL;
		break;
	}
	case SYS_SPU_IMAGE_TYPE_USER:
	{
		if (img->entry_point > 0x3fffc || img->nsegs <= 0 || img->nsegs > 0x20)
		{
			return CELL_EINVAL;
		}

		image = *img;
		break;
	}
	default: return CELL_EINVAL;
	}

	// Read thread name
	const std::string thread_name(attr->name.get_ptr(), std::max<u32>(attr->name_len, 1) - 1);

	const auto group = idm::get<lv2_spu_group>(group_id);

	if (!group)
	{
		return CELL_ESRCH;
	}

	if (spu_num >= group->threads_map.size())
	{
		return CELL_EINVAL;
	}

	std::lock_guard lock(group->mutex);

	if (group->threads_map[spu_num] != -1 || group->run_state != SPU_THREAD_GROUP_STATUS_NOT_INITIALIZED)
	{
		return CELL_EBUSY;
	}

	if (u32 option = attr->option)
	{
		sys_spu.warning("Unimplemented SPU Thread options (0x%x)", option);
	}

	const vm::addr_t ls_addr{verify("SPU LS" HERE, vm::alloc(0x80000, vm::main))};

	const u32 inited = group->init;

	const u32 tid = (inited << 24) | (group_id & 0xffffff);

	verify(HERE), idm::import<named_thread<spu_thread>>([&]()
	{
		std::string full_name = fmt::format("SPU[0x%07x] Thread", tid);

		if (!thread_name.empty())
		{
			fmt::append(full_name, " (%s)", thread_name);
		}

		const auto spu = std::make_shared<named_thread<spu_thread>>(full_name, ls_addr, group.get(), spu_num, thread_name, tid);
		group->threads[inited] = spu;
		group->threads_map[spu_num] = static_cast<s8>(inited);
		return spu;
	});

	*thread = tid;

	group->args[inited] = {arg->arg1, arg->arg2, arg->arg3, arg->arg4};
	group->imgs[inited].first = image;
	group->imgs[inited].second.assign(image.segs.get_ptr(), image.segs.get_ptr() + image.nsegs);

	if (++group->init == group->max_num)
	{
		if (g_cfg.core.max_spurs_threads < 6 && group->max_num > 0u + g_cfg.core.max_spurs_threads)
		{
			if (group->name.ends_with("CellSpursKernelGroup"))
			{
				// Hack: don't run more SPURS threads than specified.
				group->max_run = g_cfg.core.max_spurs_threads;

				spu_log.success("HACK: '%s' (0x%x) limited to %u threads.", group->name, group_id, +g_cfg.core.max_spurs_threads);
			}
		}

		group->run_state = SPU_THREAD_GROUP_STATUS_INITIALIZED;
	}

	sys_spu.warning(u8"sys_spu_thread_initialize(): Thread “%s” created (id=0x%x)", thread_name, tid);
	return CELL_OK;
}

error_code sys_spu_thread_set_argument(ppu_thread& ppu, u32 id, vm::ptr<sys_spu_thread_argument> arg)
{
	vm::temporary_unlock(ppu);

	sys_spu.warning("sys_spu_thread_set_argument(id=0x%x, arg=*0x%x)", id, arg);

	const auto [thread, group] = lv2_spu_group::get_thread(id);

	if (!thread) [[unlikely]]
	{
		return CELL_ESRCH;
	}

	std::lock_guard lock(group->mutex);

	group->args[id >> 24] = {arg->arg1, arg->arg2, arg->arg3, arg->arg4};

	return CELL_OK;
}

error_code sys_spu_thread_get_exit_status(ppu_thread& ppu, u32 id, vm::ptr<u32> status)
{
	vm::temporary_unlock(ppu);

	sys_spu.warning("sys_spu_thread_get_exit_status(id=0x%x, status=*0x%x)", id, status);

	const auto [thread, group] = lv2_spu_group::get_thread(id);

	if (!thread) [[unlikely]]
	{
		return CELL_ESRCH;
	}

	if (thread->status_npc.load().status & SPU_STATUS_STOPPED_BY_STOP)
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

	const s32 min_prio = g_ps3_process_info.has_root_perm() ? 0 : 16;

	if (attr->nsize > 0x80 || !num)
	{
		return CELL_EINVAL;
	}

	const s32 type = attr->type;

	bool use_scheduler = true;
	bool use_memct = !!(type & SYS_SPU_THREAD_GROUP_TYPE_MEMORY_FROM_CONTAINER);
	bool needs_root = false;
	u32 max_threads = 6; // TODO: max num value should be affected by sys_spu_initialize() settings
	u32 min_threads = 1;
	u32 mem_size = 0;
	lv2_memory_container* ct{};

	if (type)
	{
		sys_spu.warning("sys_spu_thread_group_create(): SPU Thread Group type (0x%x)", type);
	}

	switch (type)
	{
	case 0x0:
	case 0x4:
	case 0x18:
	{
		break;
	}

	case 0x20:
	case 0x22:
	case 0x24:
	case 0x26:
	{
		if (type == 0x22 || type == 0x26)
		{
			needs_root = true;
		}

		min_threads = 2; // That's what appears from reversing
		break;
	}

	case 0x2:
	case 0x6:
	case 0xA:

	case 0x102:
	case 0x106:
	case 0x10A:

	case 0x202:
	case 0x206:
	case 0x20A:

	case 0x902:
	case 0x906:

	case 0xA02:
	case 0xA06:

	case 0xC02:
	case 0xC06:
	{
		if (type & 0x700)
		{
			max_threads = 1;
		}

		needs_root = true;
		break;
	}
	default: return CELL_EINVAL;
	}

	if (type & SYS_SPU_THREAD_GROUP_TYPE_COOPERATE_WITH_SYSTEM)
	{
		// Constant size, unknown what it means but it's definitely not for each spu thread alone
		mem_size = 0x40000;
		use_scheduler = false;
	}
	else if (type & SYS_SPU_THREAD_GROUP_TYPE_NON_CONTEXT)
	{
		// No memory consumed
		mem_size = 0;
		use_scheduler = false;
	}
	else
	{
		// 256kb for each spu thread, probably for saving and restoring SPU LS (used by scheduler?)
		mem_size = 0x40000 * num;
	}

	if (num < min_threads || num > max_threads ||
		(needs_root && min_prio == 0x10) || (use_scheduler && (prio > 255 || prio < min_prio)))
	{
		return CELL_EINVAL;
	}

	if (use_memct && mem_size)
	{
		const auto sct = idm::get<lv2_memory_container>(attr->ct);

		if (!sct)
		{
			return CELL_ESRCH;
		}

		if (sct->take(mem_size) != mem_size)
		{
			return CELL_ENOMEM;
		}

		ct = sct.get();
	}
	else
	{
		ct = g_fxo->get<lv2_memory_container>();

		if (ct->take(mem_size) != mem_size)
		{
			return CELL_ENOMEM;
		}
	}

	const auto group = idm::make_ptr<lv2_spu_group>(std::string(attr->name.get_ptr(), std::max<u32>(attr->nsize, 1) - 1), num, prio, type, ct, use_scheduler, mem_size);

	if (!group)
	{
		ct->used -= mem_size;
		return CELL_EAGAIN;
	}

	*id = idm::last_id();
	sys_spu.warning(u8"sys_spu_thread_group_create(): Thread group “%s” created (id=0x%x)", group->name, idm::last_id());
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

		group.ct->used -= group.mem_size;
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

	for (const auto& t : group->threads)
	{
		if (auto thread = t.get())
		{
			// Remove ID from IDM (destruction will occur in group destructor)
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

	const u32 max_threads = group->max_run;

	group->join_state = 0;
	group->running = max_threads;

	for (auto& thread : group->threads)
	{
		if (thread)
		{
			auto& args = group->args[thread->lv2_id >> 24];
			auto& img = group->imgs[thread->lv2_id >> 24];

			sys_spu_image::deploy(thread->offset, img.second.data(), img.first.nsegs);

			thread->cpu_init();
			thread->gpr[3] = v128::from64(0, args[0]);
			thread->gpr[4] = v128::from64(0, args[1]);
			thread->gpr[5] = v128::from64(0, args[2]);
			thread->gpr[6] = v128::from64(0, args[3]);

			thread->status_npc = {SPU_STATUS_RUNNING, img.first.entry_point};
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

	if (!group->has_scheduler_context || group->type & 0xf00)
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

	if (!group->has_scheduler_context || group->type & 0xf00)
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

	// No effect on these group types
	if (!group->has_scheduler_context || group->type & 0xf00)
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

	const auto group = idm::get<lv2_spu_group>(id);

	if (!group)
	{
		return CELL_ESRCH;
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
		if (status)
		{
			// Report unwritten data
			return CELL_EFAULT;
		}

		return not_an_error(CELL_EFAULT);
	}

	*cause = static_cast<u32>(ppu.gpr[4]);

	if (!status)
	{
		return not_an_error(CELL_EFAULT);
	}

	*status = static_cast<s32>(ppu.gpr[5]);
	return CELL_OK;
}

error_code sys_spu_thread_group_set_priority(ppu_thread& ppu, u32 id, s32 priority)
{
	vm::temporary_unlock(ppu);

	sys_spu.trace("sys_spu_thread_group_set_priority(id=0x%x, priority=%d)", id, priority);

	const auto group = idm::get<lv2_spu_group>(id);

	if (!group)
	{
		return CELL_ESRCH;
	}

	if (!group->has_scheduler_context || priority < (g_ps3_process_info.has_root_perm() ? 0 : 16) || priority > 255)
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

	if (!group->has_scheduler_context)
	{
		*priority = 0;
	}
	else
	{
		*priority = group->prio;
	}

	return CELL_OK;
}

error_code sys_spu_thread_group_set_cooperative_victims(ppu_thread& ppu, u32 id, u32 threads_mask)
{
	vm::temporary_unlock(ppu);

	sys_spu.warning("sys_spu_thread_group_set_cooperative_victims(id=0x%x, threads_mask=0x%x)", id, threads_mask);

	const auto group = idm::get<lv2_spu_group>(id);

	if (!group)
	{
		return CELL_ESRCH;
	}

	if (!(group->type & SYS_SPU_THREAD_GROUP_TYPE_COOPERATE_WITH_SYSTEM))
	{
		return CELL_EINVAL;
	}

	if (threads_mask >= 1u << group->max_num)
	{
		return CELL_EINVAL;
	}

	// TODO

	return CELL_OK;
}

error_code sys_spu_thread_group_syscall_253(ppu_thread& ppu, u32 id, vm::ptr<sys_spu_thread_group_syscall_253_info> info)
{
	vm::temporary_unlock(ppu);

	sys_spu.warning("sys_spu_thread_group_syscall_253(id=0x%x, info=*0x%x)", id, info);

	const auto group = idm::get<lv2_spu_group>(id);

	if (!group)
	{
		return CELL_ESRCH;
	}

	if (!(group->type & SYS_SPU_THREAD_GROUP_TYPE_COOPERATE_WITH_SYSTEM))
	{
		return CELL_EINVAL;
	}

	// TODO

	info->deadlineMissCounter = 0;
	info->deadlineMeetCounter = 0;
	info->timestamp = get_timebased_time();
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

	const auto [thread, group] = lv2_spu_group::get_thread(id);

	if (!thread) [[unlikely]]
	{
		return CELL_ESRCH;
	}

	std::lock_guard lock(group->mutex);

	if (group->run_state < SPU_THREAD_GROUP_STATUS_WAITING || group->run_state > SPU_THREAD_GROUP_STATUS_RUNNING)
	{
		return CELL_ESTAT;
	}

	switch (type)
	{
	case 1: thread->_ref<u8>(lsa) = static_cast<u8>(value); break;
	case 2: thread->_ref<u16>(lsa) = static_cast<u16>(value); break;
	case 4: thread->_ref<u32>(lsa) = static_cast<u32>(value); break;
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

	const auto [thread, group] = lv2_spu_group::get_thread(id);

	if (!thread) [[unlikely]]
	{
		return CELL_ESRCH;
	}

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

	const auto [thread, group] = lv2_spu_group::get_thread(id);

	if (!thread) [[unlikely]]
	{
		return CELL_ESRCH;
	}

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

	const auto [thread, group] = lv2_spu_group::get_thread(id);

	if (!thread) [[unlikely]]
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

	const auto [thread, group] = lv2_spu_group::get_thread(id);

	if (!thread) [[unlikely]]
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

	const auto [thread, group] = lv2_spu_group::get_thread(id);

	if (!thread) [[unlikely]]
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

	if (!group)
	{
		return CELL_ESRCH;
	}

	const auto ep =
		et == SYS_SPU_THREAD_GROUP_EVENT_SYSTEM_MODULE ? &group->ep_sysmodule :
		et == SYS_SPU_THREAD_GROUP_EVENT_EXCEPTION ? &group->ep_exception :
		et == SYS_SPU_THREAD_GROUP_EVENT_RUN ? &group->ep_run :
		nullptr;

	if (!ep)
	{
		sys_spu.error("sys_spu_thread_group_connect_event(): unknown event type (%d)", et);
		return CELL_EINVAL;
	}

	if (et == SYS_SPU_THREAD_GROUP_EVENT_SYSTEM_MODULE && !(group->type & SYS_SPU_THREAD_GROUP_TYPE_COOPERATE_WITH_SYSTEM))
	{
		return CELL_EINVAL;
	}

	const auto queue = idm::get<lv2_obj, lv2_event_queue>(eq);

	std::lock_guard lock(group->mutex);

	if (!ep->expired())
	{
		return CELL_EBUSY;
	}

	// ESRCH of event queue after EBUSY
	if (!queue)
	{
		return CELL_ESRCH;
	}

	*ep = queue;
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

	const auto ep =
		et == SYS_SPU_THREAD_GROUP_EVENT_SYSTEM_MODULE ? &group->ep_sysmodule :
		et == SYS_SPU_THREAD_GROUP_EVENT_EXCEPTION ? &group->ep_exception :
		et == SYS_SPU_THREAD_GROUP_EVENT_RUN ? &group->ep_run :
		nullptr;

	if (!ep)
	{
		sys_spu.error("sys_spu_thread_group_disconnect_event(): unknown event type (%d)", et);
		return CELL_EINVAL;
	}

	std::lock_guard lock(group->mutex);

	if (ep->expired())
	{
		return CELL_EINVAL;
	}

	ep->reset();
	return CELL_OK;
}

error_code sys_spu_thread_connect_event(ppu_thread& ppu, u32 id, u32 eq, u32 et, u8 spup)
{
	vm::temporary_unlock(ppu);

	sys_spu.warning("sys_spu_thread_connect_event(id=0x%x, eq=0x%x, et=%d, spup=%d)", id, eq, et, spup);

	const auto [thread, group] = lv2_spu_group::get_thread(id);
	const auto queue = idm::get<lv2_obj, lv2_event_queue>(eq);

	if (!queue || !thread) [[unlikely]]
	{
		return CELL_ESRCH;
	}

	if (et != SYS_SPU_THREAD_EVENT_USER || spup > 63)
	{
		sys_spu.error("sys_spu_thread_connect_event(): invalid arguments (et=%d, spup=%d, queue->type=%d)", et, spup, queue->type);
		return CELL_EINVAL;
	}

	std::lock_guard lock(group->mutex);

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

	const auto [thread, group] = lv2_spu_group::get_thread(id);

	if (!thread) [[unlikely]]
	{
		return CELL_ESRCH;
	}

	if (et != SYS_SPU_THREAD_EVENT_USER || spup > 63)
	{
		sys_spu.error("sys_spu_thread_disconnect_event(): invalid arguments (et=%d, spup=%d)", et, spup);
		return CELL_EINVAL;
	}

	std::lock_guard lock(group->mutex);

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

	const auto [thread, group] = lv2_spu_group::get_thread(id);
	const auto queue = idm::get<lv2_obj, lv2_event_queue>(spuq);

	if (!queue || !thread) [[unlikely]]
	{
		return CELL_ESRCH;
	}

	if (queue->type != SYS_SPU_QUEUE)
	{
		return CELL_EINVAL;
	}

	std::lock_guard lock(group->mutex);

	decltype(std::data(thread->spuq)) q{};

	for (auto& v : thread->spuq)
	{
		// Check if the entry is assigned at all
		if (const decltype(v.second) test{};
			!v.second.owner_before(test) && !test.owner_before(v.second))
		{
			if (!q)
			{
				q = &v;
			}

			continue;
		}

		if (v.first == spuq_num ||
			(!v.second.owner_before(queue) && !queue.owner_before(v.second)))
		{
			return CELL_EBUSY;
		}
	}

	if (!q)
	{
		return CELL_EAGAIN;
	}

	q->first = spuq_num;
	q->second = queue;
	return CELL_OK;
}

error_code sys_spu_thread_unbind_queue(ppu_thread& ppu, u32 id, u32 spuq_num)
{
	vm::temporary_unlock(ppu);

	sys_spu.warning("sys_spu_thread_unbind_queue(id=0x%x, spuq_num=0x%x)", id, spuq_num);

	const auto [thread, group] = lv2_spu_group::get_thread(id);

	if (!thread) [[unlikely]]
	{
		return CELL_ESRCH;
	}

	std::lock_guard lock(group->mutex);

	for (auto& v : thread->spuq)
	{
		if (v.first != spuq_num)
		{
			continue;
		}

		if (const decltype(v.second) test{};
			!v.second.owner_before(test) && !test.owner_before(v.second))
		{
			continue;
		}

		v.second.reset();
		return CELL_OK;
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

	const auto [thread, group] = lv2_spu_group::get_thread(id);

	if (!thread) [[unlikely]]
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

	if (!thread) [[unlikely]]
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

	const u32 tid = idm::make<named_thread<spu_thread>>(fmt::format("RawSPU[0x%x] Thread", index), ls_addr, nullptr, index, "", index);

	spu_thread::g_raw_spu_id[index] = verify("RawSPU ID" HERE, tid);

	*id = index;

	return CELL_OK;
}

error_code sys_raw_spu_destroy(ppu_thread& ppu, u32 id)
{
	vm::temporary_unlock(ppu);

	sys_spu.warning("sys_raw_spu_destroy(id=%d)", id);

	const u32 idm_id = spu_thread::find_raw_spu(id);

	auto thread = idm::get<named_thread<spu_thread>>(idm_id, [](named_thread<spu_thread>& thread)
	{
		// Stop thread
		thread.state += cpu_flag::exit;
		thread = thread_state::aborting;
	});

	if (!thread) [[unlikely]]
	{
		return CELL_ESRCH;
	}

	// TODO: CELL_EBUSY is not returned

	// Kernel objects which must be removed
	std::vector<std::pair<std::shared_ptr<lv2_obj>, u32>> to_remove;

	// Clear interrupt handlers
	for (auto& intr : thread->int_ctrl)
	{
		if (auto tag = intr.tag.lock())
		{
			if (auto handler = tag->handler.lock())
			{
				// SLEEP
				lv2_obj::sleep(ppu);
				handler->join();
				to_remove.emplace_back(std::move(handler), 0);
			}

			to_remove.emplace_back(std::move(tag), 0);
		}
	}

	// Scan all kernel objects to determine IDs
	idm::select<lv2_obj>([&](u32 id, lv2_obj& obj)
	{
		for (auto& pair : to_remove)
		{
			if (pair.first.get() == std::addressof(obj))
			{
				pair.second = id;
			}
		}
	});

	// Remove IDs
	for (auto&& pair : to_remove)
	{
		if (pair.second >> 24 == 0xa)
			idm::remove_verify<lv2_obj, lv2_int_tag>(pair.second, std::move(pair.first));
		if (pair.second >> 24 == 0xb)
			idm::remove_verify<lv2_obj, lv2_int_serv>(pair.second, std::move(pair.first));
	}

	(*thread)();

	if (!idm::remove_verify<named_thread<spu_thread>>(idm_id, std::move(thread)))
	{
		// Other thread destroyed beforehead
		return CELL_ESRCH;
	}

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

		if (!thread || *thread == thread_state::aborting)
		{
			error = CELL_ESRCH;
			return result;
		}

		auto& int_ctrl = thread->int_ctrl[class_id];

		if (!int_ctrl.tag.expired())
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

	if (!thread) [[unlikely]]
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

	if (!thread) [[unlikely]]
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

	if (!thread) [[unlikely]]
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

	if (!thread) [[unlikely]]
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

	if (!thread) [[unlikely]]
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

	if (!thread) [[unlikely]]
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

	if (!thread) [[unlikely]]
	{
		return CELL_ESRCH;
	}

	*value = static_cast<u32>(thread->snr_config);

	return CELL_OK;
}
