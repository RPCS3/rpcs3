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
#include "Emu/Cell/timers.hpp"
#include "Emu/Memory/vm_reservation.h"
#include "sys_interrupt.h"
#include "sys_process.h"
#include "sys_memory.h"
#include "sys_mmapper.h"
#include "sys_event.h"
#include "sys_fs.h"

#include "util/asm.hpp"

LOG_CHANNEL(sys_spu);

template <>
void fmt_class_string<spu_group_status>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](spu_group_status value)
	{
		switch (value)
		{
		case SPU_THREAD_GROUP_STATUS_NOT_INITIALIZED: return "uninitialized";
		case SPU_THREAD_GROUP_STATUS_INITIALIZED: return "initialized";
		case SPU_THREAD_GROUP_STATUS_READY: return "ready";
		case SPU_THREAD_GROUP_STATUS_WAITING: return "waiting";
		case SPU_THREAD_GROUP_STATUS_SUSPENDED: return "suspended";
		case SPU_THREAD_GROUP_STATUS_WAITING_AND_SUSPENDED: return "waiting and suspended";
		case SPU_THREAD_GROUP_STATUS_RUNNING: return "running";
		case SPU_THREAD_GROUP_STATUS_STOPPED: return "stopped";
		case SPU_THREAD_GROUP_STATUS_DESTROYED: return "destroyed";
		case SPU_THREAD_GROUP_STATUS_UNKNOWN: break;
		}

		return unknown;
	});
}

template <>
void fmt_class_string<spu_stop_syscall>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](spu_stop_syscall value)
	{
		switch (value)
		{
		case SYS_SPU_THREAD_STOP_YIELD: return "sys_spu_thread_yield";
		case SYS_SPU_THREAD_STOP_GROUP_EXIT: return "sys_spu_thread_group_exit";
		case SYS_SPU_THREAD_STOP_THREAD_EXIT: return "sys_spu_thread_thread_exit";
		case SYS_SPU_THREAD_STOP_RECEIVE_EVENT: return "sys_spu_thread_receive_event";
		case SYS_SPU_THREAD_STOP_TRY_RECEIVE_EVENT: return "sys_spu_thread_tryreceive_event";
		case SYS_SPU_THREAD_STOP_SWITCH_SYSTEM_MODULE: return "sys_spu_thread_switch_system_module";
		}

		return unknown;
	});
}

bool sys_spu_image::load(const fs::file& stream)
{
	const spu_exec_object obj{stream, 0, elf_opt::no_sections + elf_opt::no_data};

	if (obj != elf_error::ok)
	{
		sys_spu.error("Failed to load SPU image: %s", obj.get_error());
		return false;
	}

	for (const auto& shdr : obj.shdrs)
	{
		spu_log.notice("** Section: sh_type=0x%x, addr=0x%llx, size=0x%llx, flags=0x%x", std::bit_cast<u32>(shdr.sh_type), shdr.sh_addr, shdr.sh_size, shdr._sh_flags);
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
	const vm::ptr<sys_spu_segment> segs = vm::cast(vm::reserve_map(vm::user64k, 0, 0x10000000)->alloc(mem_size));

	//const u32 entry = obj.header.e_entry;

	const u32 src = (segs + nsegs).addr();

	stream.seek(0);
	stream.read(vm::base(src), stream.size());

	if (nsegs <= 0 || nsegs > 0x20 || sys_spu_image::fill(segs, nsegs, obj.progs, src) != nsegs)
	{
		fmt::throw_exception("Failed to load SPU segments (%d)", nsegs);
	}

	// Write ID and save entry
	this->entry_point = idm::make<lv2_obj, lv2_spu_image>(+obj.header.e_entry, segs, nsegs);

	// Unused and set to 0
	this->nsegs = 0;
	this->segs = vm::null;

	vm::page_protect(segs.addr(), utils::align(mem_size, 4096), 0, 0, vm::page_writable);
	return true;
}

void sys_spu_image::free() const
{
	if (type == SYS_SPU_IMAGE_TYPE_KERNEL)
	{
		// TODO: Remove, should be handled by syscalls
		ensure(vm::dealloc(segs.addr(), vm::main));
	}
}

void sys_spu_image::deploy(u8* loc, std::span<const sys_spu_segment> segs, bool is_verbose)
{
	// Segment info dump
	std::string dump;

	// Executable hash
	sha1_context sha;
	sha1_starts(&sha);
	u8 sha1_hash[20];

	for (const auto& seg : segs)
	{
		fmt::append(dump, "\n\t[%u] t=0x%x, ls=0x%x, size=0x%x, addr=0x%x", &seg - segs.data(), seg.type, seg.ls, seg.size, seg.addr);

		sha1_update(&sha, reinterpret_cast<const uchar*>(&seg.type), sizeof(seg.type));

		// Hash big-endian values
		if (seg.type == SYS_SPU_SEGMENT_TYPE_COPY)
		{
			std::memcpy(loc + seg.ls, vm::base(seg.addr), seg.size);
			sha1_update(&sha, reinterpret_cast<const uchar*>(&seg.size), sizeof(seg.size));
			sha1_update(&sha, reinterpret_cast<const uchar*>(&seg.ls), sizeof(seg.ls));
			sha1_update(&sha, vm::_ptr<uchar>(seg.addr), seg.size);
		}
		else if (seg.type == SYS_SPU_SEGMENT_TYPE_FILL)
		{
			if ((seg.ls | seg.size) % 4)
			{
				spu_log.error("Unaligned SPU FILL type segment (ls=0x%x, size=0x%x)", seg.ls, seg.size);
			}

			std::fill_n(reinterpret_cast<be_t<u32>*>(loc + seg.ls), seg.size / 4, seg.addr);
			sha1_update(&sha, reinterpret_cast<const uchar*>(&seg.size), sizeof(seg.size));
			sha1_update(&sha, reinterpret_cast<const uchar*>(&seg.ls), sizeof(seg.ls));
			sha1_update(&sha, reinterpret_cast<const uchar*>(&seg.addr), sizeof(seg.addr));
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

	auto mem_translate = [loc](u32 addr, u32 size)
	{
		return utils::add_saturate<u32>(addr, size) <= SPU_LS_SIZE ? loc + addr : nullptr;
	};

	// Apply the patch
	std::vector<u32> applied;
	g_fxo->get<patch_engine>().apply(applied, hash, mem_translate);

	if (!Emu.GetTitleID().empty())
	{
		// Alternative patch
		g_fxo->get<patch_engine>().apply(applied, Emu.GetTitleID() + '-' + hash, mem_translate);
	}

	(is_verbose ? spu_log.notice : sys_spu.trace)("Loaded SPU image: %s (<- %u)%s", hash, applied.size(), dump);
}

lv2_spu_group::lv2_spu_group(utils::serial& ar) noexcept
	: name(ar.pop<std::string>())
	, id(idm::last_id())
	, max_num(ar)
	, mem_size(ar)
	, type(ar) // SPU Thread Group Type
	, ct(lv2_memory_container::search(ar))
	, has_scheduler_context(ar.pop<u8>())
	, max_run(ar)
	, init(ar)
	, prio([&ar]()
	{
		std::common_type_t<decltype(lv2_spu_group::prio)> prio{};

		ar(prio.all);

		return prio;
	}())
	, run_state(ar.pop<spu_group_status>())
	, exit_status(ar)
{
	for (auto& thread : threads)
	{
		if (ar.pop<bool>())
		{
			ar(id_manager::g_id);
			thread = stx::make_shared<named_thread<spu_thread>>(stx::launch_retainer{}, ar, this);
			ensure(idm::import_existing<named_thread<spu_thread>>(thread, idm::last_id()));
			running += !thread->stop_flag_removal_protection;
		}
	}

	ar(threads_map);
	ar(imgs);
	ar(args);

	for (auto ep : {&ep_run, &ep_exception, &ep_sysmodule})
	{
		*ep = idm::get_unlocked<lv2_obj, lv2_event_queue>(ar.pop<u32>());
	}

	waiter_spu_index = -1;

	switch (run_state)
	{
	// Commented stuff are handled by different means currently
	//case SPU_THREAD_GROUP_STATUS_NOT_INITIALIZED:
	//case SPU_THREAD_GROUP_STATUS_INITIALIZED:
	//case SPU_THREAD_GROUP_STATUS_READY:
	case SPU_THREAD_GROUP_STATUS_WAITING:
	{
		run_state = SPU_THREAD_GROUP_STATUS_RUNNING;
		ar(waiter_spu_index);
		[[fallthrough]];
	}
	case SPU_THREAD_GROUP_STATUS_WAITING_AND_SUSPENDED:
	{
		if (run_state == SPU_THREAD_GROUP_STATUS_WAITING_AND_SUSPENDED)
		{
			run_state = SPU_THREAD_GROUP_STATUS_SUSPENDED;
		}

		[[fallthrough]];
	}
	case SPU_THREAD_GROUP_STATUS_SUSPENDED:
	{
		// Suspend all SPU threads except a thread that waits on sys_spu_thread_receive_event
		for (const auto& thread : threads)
		{
			if (thread)
			{
				if (thread->index == waiter_spu_index)
				{
					lv2_obj::set_future_sleep(thread.get());
					continue;
				}

				thread->state += cpu_flag::suspend;
			}
		}

		break;
	}
	//case SPU_THREAD_GROUP_STATUS_RUNNING:
	//case SPU_THREAD_GROUP_STATUS_STOPPED:
	//case SPU_THREAD_GROUP_STATUS_UNKNOWN:
	default:
	{
		break;
	}
	}
}

void lv2_spu_group::save(utils::serial& ar)
{
	USING_SERIALIZATION_VERSION(spu);

	ar(name, max_num, mem_size, type, ct->id, has_scheduler_context, max_run, init, prio.load().all, run_state, exit_status);

	for (const auto& thread : threads)
	{
		ar(u8{thread.operator bool()});

		if (thread)
		{
			ar(thread->id);
			thread->save(ar);
		}
	}

	ar(threads_map);
	ar(imgs);
	ar(args);

	for (auto ep : {&ep_run, &ep_exception, &ep_sysmodule})
	{
		ar(lv2_obj::check(*ep) ? (*ep)->id : 0);
	}

	if (run_state == SPU_THREAD_GROUP_STATUS_WAITING)
	{
		ar(waiter_spu_index);
	}
}

lv2_spu_image::lv2_spu_image(utils::serial& ar)
	: e_entry(ar)
	, segs(ar.pop<decltype(segs)>())
	, nsegs(ar)
{
}

void lv2_spu_image::save(utils::serial& ar)
{
	ar(e_entry, segs, nsegs);
}

// Get spu thread ptr, returns group ptr as well for refcounting
std::pair<named_thread<spu_thread>*, shared_ptr<lv2_spu_group>> lv2_spu_group::get_thread(u32 id)
{
	if (id >= 0x06000000)
	{
		// thread index is out of range (5 max)
		return {};
	}

	// Bits 0-23 contain group id (without id base)
	decltype(get_thread(0)) res{nullptr, idm::get_unlocked<lv2_spu_group>((id & 0xFFFFFF) | (lv2_spu_group::id_base & ~0xFFFFFF))};

	// Bits 24-31 contain thread index within the group
	const u32 index = id >> 24;

	if (auto group = res.second.get(); group && group->init > index)
	{
		res.first = group->threads[index].get();
	}

	return res;
}

struct limits_data
{
	u32 physical = 0;
	u32 raw_spu = 0;
	u32 controllable = 0;
	u32 spu_limit = umax;
	u32 raw_limit = umax;
};

struct spu_limits_t
{
	u32 max_raw = 0;
	u32 max_spu = 6;
	shared_mutex mutex;

	spu_limits_t() = default;

	spu_limits_t(utils::serial& ar) noexcept
	{
		ar(max_raw, max_spu);
	}

	void save(utils::serial& ar)
	{
		ar(max_raw, max_spu);
	}

	SAVESTATE_INIT_POS(47);

	bool check_valid(const limits_data& init) const
	{
		const u32 physical_spus_count = init.physical;
		const u32 controllable_spu_count = init.controllable;

		const u32 spu_limit = init.spu_limit != umax ? init.spu_limit : max_spu;
		const u32 raw_limit = init.raw_limit != umax ? init.raw_limit : max_raw;

		if (spu_limit + raw_limit > 6 || physical_spus_count > spu_limit || controllable_spu_count > spu_limit)
		{
			return false;
		}

		return true;
	}

	bool check_busy(const limits_data& init) const
	{
		u32 physical_spus_count = init.physical;
		u32 raw_spu_count = init.raw_spu;
		u32 controllable_spu_count = init.controllable;
		u32 system_coop = init.controllable != 0 && init.physical != 0 ? 1 : 0;

		const u32 spu_limit = init.spu_limit != umax ? init.spu_limit : max_spu;
		const u32 raw_limit = init.raw_limit != umax ? init.raw_limit : max_raw;

		idm::select<lv2_spu_group>([&](u32, lv2_spu_group& group)
		{
			if (group.type & SYS_SPU_THREAD_GROUP_TYPE_COOPERATE_WITH_SYSTEM)
			{
				system_coop++;
				controllable_spu_count = std::max<u32>(controllable_spu_count, 1);
				physical_spus_count += group.max_num - 1;
			}
			else if (group.has_scheduler_context)
			{
				controllable_spu_count = std::max<u32>(controllable_spu_count, group.max_num);
			}
			else
			{
				physical_spus_count += group.max_num;
			}
		});

		raw_spu_count += spu_thread::g_raw_spu_ctr;

		// physical_spus_count >= spu_limit returns EBUSY, not EINVAL!
		if (spu_limit + raw_limit > 6 || raw_spu_count > raw_limit || physical_spus_count >= spu_limit || physical_spus_count > spu_limit || controllable_spu_count > spu_limit)
		{
			return false;
		}

		if (system_coop > 1)
		{
			// Cannot have more than one SYS_SPU_THREAD_GROUP_TYPE_COOPERATE_WITH_SYSTEM group at a time
			return false;
		}

		return true;
	}
};

error_code sys_spu_initialize(ppu_thread& ppu, u32 max_usable_spu, u32 max_raw_spu)
{
	ppu.state += cpu_flag::wait;

	sys_spu.warning("sys_spu_initialize(max_usable_spu=%d, max_raw_spu=%d)", max_usable_spu, max_raw_spu);

	auto& limits = g_fxo->get<spu_limits_t>();

	if (max_raw_spu > 5)
	{
		return CELL_EINVAL;
	}

	// NOTE: This value can be changed by VSH in theory
	max_usable_spu = 6;

	std::lock_guard lock(limits.mutex);

	if (!limits.check_busy(limits_data{.spu_limit = max_usable_spu - max_raw_spu, .raw_limit = max_raw_spu}))
	{
		return CELL_EBUSY;
	}

	limits.max_raw = max_raw_spu;
	limits.max_spu = max_usable_spu - max_raw_spu;
	return CELL_OK;
}

error_code _sys_spu_image_get_information(ppu_thread& ppu, vm::ptr<sys_spu_image> img, vm::ptr<u32> entry_point, vm::ptr<s32> nsegs)
{
	ppu.state += cpu_flag::wait;

	sys_spu.warning("_sys_spu_image_get_information(img=*0x%x, entry_point=*0x%x, nsegs=*0x%x)", img, entry_point, nsegs);

	if (img->type != SYS_SPU_IMAGE_TYPE_KERNEL)
	{
		return CELL_EINVAL;
	}

	const auto image = idm::get_unlocked<lv2_obj, lv2_spu_image>(img->entry_point);

	if (!image)
	{
		return CELL_ESRCH;
	}

	ppu.check_state();
	*entry_point = image->e_entry;
	*nsegs       = image->nsegs;
	return CELL_OK;
}

error_code sys_spu_image_open(ppu_thread& ppu, vm::ptr<sys_spu_image> img, vm::cptr<char> path)
{
	ppu.state += cpu_flag::wait;

	sys_spu.warning("sys_spu_image_open(img=*0x%x, path=%s)", img, path);

	auto [fs_error, ppath, path0, file, type] = lv2_file::open(path.get_ptr(), 0, 0);

	if (fs_error)
	{
		return {fs_error, path};
	}

	u128 klic = g_fxo->get<loaded_npdrm_keys>().last_key();

	const fs::file elf_file = decrypt_self(file, reinterpret_cast<const u8*>(&klic));

	if (!elf_file || !img->load(elf_file))
	{
		sys_spu.error("sys_spu_image_open(): file %s is illegal for SPU image!", path);
		return {CELL_ENOEXEC, path};
	}

	return CELL_OK;
}

error_code sys_spu_image_open_by_fd(ppu_thread& ppu, vm::ptr<sys_spu_image> img, s32 fd, s64 offset)
{
	ppu.state += cpu_flag::wait;

	sys_spu.warning("sys_spu_image_open_by_fd(img=*0x%x, fd=%d, offset=0x%x)", img, fd, offset);

	const auto file = idm::get_unlocked<lv2_fs_object, lv2_file>(fd);

	if (!file)
	{
		return CELL_EBADF;
	}

	if (offset < 0)
	{
		return CELL_ENOEXEC;
	}

	std::lock_guard lock(file->mp->mutex);

	if (!file->file)
	{
		return CELL_EBADF;
	}

	u128 klic = g_fxo->get<loaded_npdrm_keys>().last_key();

	const fs::file elf_file = decrypt_self(lv2_file::make_view(file, offset), reinterpret_cast<const u8*>(&klic));

	if (!img->load(elf_file))
	{
		sys_spu.error("sys_spu_image_open(): file %s is illegal for SPU image!", file->name.data());
		return {CELL_ENOEXEC, file->name.data()};
	}

	return CELL_OK;
}

error_code _sys_spu_image_import(ppu_thread& ppu, vm::ptr<sys_spu_image> img, u32 src, u32 size, u32 arg4)
{
	ppu.state += cpu_flag::wait;

	sys_spu.warning("_sys_spu_image_import(img=*0x%x, src=*0x%x, size=0x%x, arg4=0x%x)", img, src, size, arg4);

	img->load(fs::file{vm::base(src), size});
	return CELL_OK;
}

error_code _sys_spu_image_close(ppu_thread& ppu, vm::ptr<sys_spu_image> img)
{
	ppu.state += cpu_flag::wait;

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

	ensure(vm::dealloc(handle->segs.addr(), vm::user64k));
	return CELL_OK;
}

error_code _sys_spu_image_get_segments(ppu_thread& ppu, vm::ptr<sys_spu_image> img, vm::ptr<sys_spu_segment> segments, s32 nseg)
{
	ppu.state += cpu_flag::wait;

	sys_spu.error("_sys_spu_image_get_segments(img=*0x%x, segments=*0x%x, nseg=%d)", img, segments, nseg);

	if (nseg <= 0 || nseg > 0x20 || img->type != SYS_SPU_IMAGE_TYPE_KERNEL)
	{
		return CELL_EINVAL;
	}

	const auto handle = idm::get_unlocked<lv2_obj, lv2_spu_image>(img->entry_point);

	if (!handle)
	{
		return CELL_ESRCH;
	}

	// TODO: apply SPU patches
	ppu.check_state();
	std::memcpy(segments.get_ptr(), handle->segs.get_ptr(), sizeof(sys_spu_segment) * std::min<s32>(nseg, handle->nsegs));
	return CELL_OK;
}

error_code sys_spu_thread_initialize(ppu_thread& ppu, vm::ptr<u32> thread, u32 group_id, u32 spu_num, vm::ptr<sys_spu_image> img, vm::ptr<sys_spu_thread_attribute> attr, vm::ptr<sys_spu_thread_argument> arg)
{
	ppu.state += cpu_flag::wait;

	sys_spu.warning("sys_spu_thread_initialize(thread=*0x%x, group=0x%x, spu_num=%d, img=*0x%x, attr=*0x%x, arg=*0x%x)", thread, group_id, spu_num, img, attr, arg);

	if (spu_num >= std::size(decltype(lv2_spu_group::threads_map){}))
	{
		return CELL_EINVAL;
	}

	if (!attr)
	{
		return CELL_EFAULT;
	}

	const sys_spu_thread_attribute attr_data = *attr;

	if (attr_data.name_len > 0x80)
	{
		return CELL_EINVAL;
	}

	if (!arg)
	{
		return CELL_EFAULT;
	}

	const sys_spu_thread_argument args = *arg;
	const u32 option = attr_data.option;

	if (option & ~(SYS_SPU_THREAD_OPTION_DEC_SYNC_TB_ENABLE | SYS_SPU_THREAD_OPTION_ASYNC_INTR_ENABLE))
	{
		return CELL_EINVAL;
	}

	if (!img)
	{
		return CELL_EFAULT;
	}

	sys_spu_image image = *img;

	switch (image.type)
	{
	case SYS_SPU_IMAGE_TYPE_KERNEL:
	{
		const auto handle = idm::get_unlocked<lv2_obj, lv2_spu_image>(image.entry_point);

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
		if (image.entry_point > 0x3fffc || image.nsegs <= 0 || image.nsegs > 0x20)
		{
			return CELL_EINVAL;
		}

		break;
	}
	default: return CELL_EINVAL;
	}

	std::vector<sys_spu_segment> spu_segs(image.segs.get_ptr(), image.segs.get_ptr() + image.nsegs);

	bool found_info_segment = false;
	bool found_copy_segment = false;

	for (const auto& seg : spu_segs)
	{
		if (image.type == SYS_SPU_IMAGE_TYPE_KERNEL)
		{
			// Assume valid, values are coming from LV2
			found_copy_segment = true;
			break;
		}

		switch (seg.type)
		{
		case SYS_SPU_SEGMENT_TYPE_COPY:
		{
			if (seg.addr % 4)
			{
				// 4-bytes unaligned address is not valid
				return CELL_EINVAL;
			}

			found_copy_segment = true;
			break;
		}
		case SYS_SPU_SEGMENT_TYPE_FILL:
		{
			break;
		}
		case SYS_SPU_SEGMENT_TYPE_INFO:
		{
			// There can only be one INFO segment at max
			if (seg.size > 256u || found_info_segment)
			{
				return CELL_EINVAL;
			}

			found_info_segment = true;
			continue;
		}
		default: return CELL_EINVAL;
		}

		if (!seg.size || (seg.ls | seg.size) % 0x10 || seg.ls >= SPU_LS_SIZE || seg.size > SPU_LS_SIZE)
		{
			return CELL_EINVAL;
		}

		for (auto it = spu_segs.data(); it != &seg; it++)
		{
			if (it->type != SYS_SPU_SEGMENT_TYPE_INFO)
			{
				if (seg.ls + seg.size > it->ls && it->ls + it->size > seg.ls)
				{
					// Overlapping segments are not allowed
					return CELL_EINVAL;
				}
			}
		}
	}

	// There must be at least one COPY segment
	if (!found_copy_segment)
	{
		return CELL_EINVAL;
	}

	// Read thread name
	const std::string thread_name(attr_data.name.get_ptr(), std::max<u32>(attr_data.name_len, 1) - 1);

	const auto group = idm::get_unlocked<lv2_spu_group>(group_id);

	if (!group)
	{
		return CELL_ESRCH;
	}

	std::unique_lock lock(group->mutex);

	if (auto state = +group->run_state; state != SPU_THREAD_GROUP_STATUS_NOT_INITIALIZED)
	{
		if (state == SPU_THREAD_GROUP_STATUS_DESTROYED)
		{
			return CELL_ESRCH;
		}

		return CELL_EBUSY;
	}

	if (group->threads_map[spu_num] != -1)
	{
		return CELL_EBUSY;
	}

	if (option & SYS_SPU_THREAD_OPTION_ASYNC_INTR_ENABLE)
	{
		sys_spu.warning("Unimplemented SPU Thread options (0x%x)", option);
	}

	const u32 inited = group->init;

	const u32 tid = (inited << 24) | (group_id & 0xffffff);

	ensure(idm::import<named_thread<spu_thread>>([&]()
	{
		const auto spu = stx::make_shared<named_thread<spu_thread>>(group.get(), spu_num, thread_name, tid, false, option);
		group->threads[inited] = spu;
		group->threads_map[spu_num] = static_cast<s8>(inited);
		return spu;
	}));

	// alloc_hidden indicates falloc to allocate page with no access rights in base memory
	auto& spu = group->threads[inited];
	ensure(vm::get(vm::spu)->falloc(spu->vm_offset(), SPU_LS_SIZE, &spu->shm, static_cast<u64>(vm::page_size_64k) | static_cast<u64>(vm::alloc_hidden)));
	spu->map_ls(*spu->shm, spu->ls);

	group->args[inited] = {args.arg1, args.arg2, args.arg3, args.arg4};
	group->imgs[inited].first = image.entry_point;
	group->imgs[inited].second = std::move(spu_segs);

	if (++group->init == group->max_num)
	{
		const auto old = group->run_state.compare_and_swap(SPU_THREAD_GROUP_STATUS_NOT_INITIALIZED, SPU_THREAD_GROUP_STATUS_INITIALIZED);

		if (old == SPU_THREAD_GROUP_STATUS_DESTROYED)
		{
			return CELL_ESRCH;
		}
	}

	lock.unlock();
	sys_spu.warning("sys_spu_thread_initialize(): Thread \"%s\" created (id=0x%x)", thread_name, tid);

	ppu.check_state();
	*thread = tid;
	return CELL_OK;
}

error_code sys_spu_thread_set_argument(ppu_thread& ppu, u32 id, vm::ptr<sys_spu_thread_argument> arg)
{
	ppu.state += cpu_flag::wait;

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

error_code sys_spu_thread_get_exit_status(ppu_thread& ppu, u32 id, vm::ptr<s32> status)
{
	ppu.state += cpu_flag::wait;

	sys_spu.warning("sys_spu_thread_get_exit_status(id=0x%x, status=*0x%x)", id, status);

	const auto [thread, group] = lv2_spu_group::get_thread(id);

	if (!thread) [[unlikely]]
	{
		return CELL_ESRCH;
	}

	u32 data;

	if (thread->exit_status.try_read(data))
	{
		ppu.check_state();
		*status = static_cast<s32>(data);
		return CELL_OK;
	}

	return CELL_ESTAT;
}

error_code sys_spu_thread_group_create(ppu_thread& ppu, vm::ptr<u32> id, u32 num, s32 prio, vm::ptr<sys_spu_thread_group_attribute> attr)
{
	ppu.state += cpu_flag::wait;

	sys_spu.warning("sys_spu_thread_group_create(id=*0x%x, num=%d, prio=%d, attr=*0x%x)", id, num, prio, attr);

	const s32 min_prio = g_ps3_process_info.has_root_perm() ? 0 : 16;

	const sys_spu_thread_group_attribute attr_data = *attr;

	if (attr_data.nsize > 0x80 || !num)
	{
		return CELL_EINVAL;
	}

	const s32 type = attr_data.type;

	bool use_scheduler = true;
	bool use_memct = !!(type & SYS_SPU_THREAD_GROUP_TYPE_MEMORY_FROM_CONTAINER);
	bool needs_root = false;
	u32 max_threads = 6;
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
	case SYS_SPU_THREAD_GROUP_TYPE_MEMORY_FROM_CONTAINER:
	case SYS_SPU_THREAD_GROUP_TYPE_EXCLUSIVE_NON_CONTEXT:
	{
		break;
	}

	case SYS_SPU_THREAD_GROUP_TYPE_COOPERATE_WITH_SYSTEM:
	case (SYS_SPU_THREAD_GROUP_TYPE_COOPERATE_WITH_SYSTEM | 0x2):
	case (SYS_SPU_THREAD_GROUP_TYPE_COOPERATE_WITH_SYSTEM | 0x4):
	case (SYS_SPU_THREAD_GROUP_TYPE_COOPERATE_WITH_SYSTEM | 0x6):
	{
		if (type == 0x22 || type == 0x26)
		{
			needs_root = true;
		}

		// For a single thread that is being shared with system (the cooperative victim)
		min_threads = 2;
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

	const bool is_system_coop = type & SYS_SPU_THREAD_GROUP_TYPE_COOPERATE_WITH_SYSTEM;

	if (is_system_coop)
	{
		// For a single thread that is being shared with system (the cooperative victim)
		mem_size = SPU_LS_SIZE;
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
		mem_size = SPU_LS_SIZE * num;
	}

	if (num < min_threads || num > max_threads ||
		(needs_root && min_prio == 0x10) || (use_scheduler && !is_system_coop && (prio > 255 || prio < min_prio)))
	{
		return CELL_EINVAL;
	}

	if (use_memct && mem_size)
	{
		const auto sct = idm::get_unlocked<lv2_memory_container>(attr_data.ct);

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
		ct = &g_fxo->get<lv2_memory_container>();

		if (ct->take(mem_size) != mem_size)
		{
			return CELL_ENOMEM;
		}
	}

	auto& limits = g_fxo->get<spu_limits_t>();

	std::unique_lock lock(limits.mutex);

	limits_data group_limits{};

	if (is_system_coop)
	{
		group_limits.controllable = 1;
		group_limits.physical = num - 1;
	}
	else if (use_scheduler)
	{
		group_limits.controllable = num;
	}
	else
	{
		group_limits.physical = num;
	}

	if (!limits.check_valid(group_limits))
	{
		ct->free(mem_size);
		return CELL_EINVAL;
	}

	if (!limits.check_busy(group_limits))
	{
		ct->free(mem_size);
		return CELL_EBUSY;
	}

	const auto group = idm::make_ptr<lv2_spu_group>(std::string(attr_data.name.get_ptr(), std::max<u32>(attr_data.nsize, 1) - 1), num, prio, type, ct, use_scheduler, mem_size);

	if (!group)
	{
		ct->free(mem_size);
		return CELL_EAGAIN;
	}

	lock.unlock();
	sys_spu.warning("sys_spu_thread_group_create(): Thread group \"%s\" created (id=0x%x)", group->name, idm::last_id());

	ppu.check_state();
	*id = idm::last_id();
	return CELL_OK;
}

error_code sys_spu_thread_group_destroy(ppu_thread& ppu, u32 id)
{
	ppu.state += cpu_flag::wait;

	sys_spu.warning("sys_spu_thread_group_destroy(id=0x%x)", id);

	auto& limits = g_fxo->get<spu_limits_t>();

	std::lock_guard lock(limits.mutex);

	const auto group = idm::withdraw<lv2_spu_group>(id, [](lv2_spu_group& group) -> CellError
	{
		if (!group.run_state.fetch_op([](spu_group_status& state)
		{
			if (state <= SPU_THREAD_GROUP_STATUS_INITIALIZED)
			{
 				state = SPU_THREAD_GROUP_STATUS_DESTROYED;
				return true;
			}

			return false;
		}).second)
		{
			return CELL_EBUSY;
		}

		group.ct->free(group.mem_size);
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

	group->mutex.lock_unlock();

	for (const auto& t : group->threads)
	{
		if (auto thread = t.get())
		{
			// Deallocate LS
			thread->cleanup();

			// Remove ID from IDM (destruction will occur in group destructor)
			idm::remove<named_thread<spu_thread>>(thread->id);
		}
	}

	return CELL_OK;
}

error_code sys_spu_thread_group_start(ppu_thread& ppu, u32 id)
{
	ppu.state += cpu_flag::wait;

	sys_spu.trace("sys_spu_thread_group_start(id=0x%x)", id);

	const auto group = idm::get_unlocked<lv2_spu_group>(id);

	if (!group)
	{
		return CELL_ESRCH;
	}

	struct notify_on_exit
	{
		usz index = umax;
		std::array<spu_thread*, 8> threads; // Raw pointer suffices, as long as group is referenced its SPUs exist

		~notify_on_exit() noexcept
		{
			for (; index != umax; index--)
			{
				threads[index]->state.notify_one();
			}
		}
	} notify_threads;

	std::lock_guard lock(group->mutex);

	// SPU_THREAD_GROUP_STATUS_READY state is not used
	switch (group->run_state.compare_and_swap(SPU_THREAD_GROUP_STATUS_INITIALIZED, SPU_THREAD_GROUP_STATUS_RUNNING))
	{
	case SPU_THREAD_GROUP_STATUS_INITIALIZED: break;
	case SPU_THREAD_GROUP_STATUS_DESTROYED: return CELL_ESRCH;
	default: return CELL_ESTAT;
	}

	const u32 max_threads = group->max_num;

	group->join_state = 0;
	group->exit_status = 0;
	group->running = max_threads;
	group->set_terminate = false;

	for (auto& thread : group->threads)
	{
		if (thread)
		{
			auto& args = group->args[thread->lv2_id >> 24];
			auto& img = group->imgs[thread->lv2_id >> 24];

			sys_spu_image::deploy(thread->ls, std::span(img.second.data(), img.second.size()), group->stop_count < 5);

			thread->cpu_init();
			thread->gpr[3] = v128::from64(0, args[0]);
			thread->gpr[4] = v128::from64(0, args[1]);
			thread->gpr[5] = v128::from64(0, args[2]);
			thread->gpr[6] = v128::from64(0, args[3]);

			thread->status_npc = {SPU_STATUS_RUNNING, img.first};
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
			notify_threads.threads[++notify_threads.index] = thread.get();
		}
	}

	return CELL_OK;
}

error_code sys_spu_thread_group_suspend(ppu_thread& ppu, u32 id)
{
	ppu.state += cpu_flag::wait;

	sys_spu.trace("sys_spu_thread_group_suspend(id=0x%x)", id);

	const auto group = idm::get_unlocked<lv2_spu_group>(id);

	if (!group)
	{
		return CELL_ESRCH;
	}

	if (!group->has_scheduler_context || group->type & 0xf00)
	{
		return CELL_EINVAL;
	}

	if (group->type & SYS_SPU_THREAD_GROUP_TYPE_COOPERATE_WITH_SYSTEM)
	{
		return CELL_EINVAL;
	}

	std::lock_guard lock(group->mutex);

	CellError error;

	group->run_state.fetch_op([&error](spu_group_status& state)
	{
		if (state == SPU_THREAD_GROUP_STATUS_DESTROYED)
		{
			error = CELL_ESRCH;
			return false;
		}

		if (state <= SPU_THREAD_GROUP_STATUS_INITIALIZED || state == SPU_THREAD_GROUP_STATUS_STOPPED)
		{
			error = CELL_ESTAT;
			return false;
		}

		// SPU_THREAD_GROUP_STATUS_READY state is not used

		if (state == SPU_THREAD_GROUP_STATUS_RUNNING)
		{
			state = SPU_THREAD_GROUP_STATUS_SUSPENDED;
		}
		else if (state == SPU_THREAD_GROUP_STATUS_WAITING)
		{
			state = SPU_THREAD_GROUP_STATUS_WAITING_AND_SUSPENDED;
		}
		else if (state == SPU_THREAD_GROUP_STATUS_SUSPENDED || state == SPU_THREAD_GROUP_STATUS_WAITING_AND_SUSPENDED)
		{
			error = {};
			return false;
		}
		else
		{

			error = CELL_ESTAT;
			return false;
		}

		error = CellError{CELL_CANCEL + 0u};
		return true;
	});

	if (error != CELL_CANCEL + 0u)
	{
		if (!error)
		{
			return CELL_OK;
		}

		return error;
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
	ppu.state += cpu_flag::wait;

	sys_spu.trace("sys_spu_thread_group_resume(id=0x%x)", id);

	const auto group = idm::get_unlocked<lv2_spu_group>(id);

	if (!group)
	{
		return CELL_ESRCH;
	}

	if (!group->has_scheduler_context || group->type & 0xf00)
	{
		return CELL_EINVAL;
	}

	if (group->type & SYS_SPU_THREAD_GROUP_TYPE_COOPERATE_WITH_SYSTEM)
	{
		return CELL_EINVAL;
	}

	struct notify_on_exit
	{
		usz index = umax;
		std::array<spu_thread*, 8> threads; // Raw pointer suffices, as long as group is referenced its SPUs exist

		~notify_on_exit() noexcept
		{
			for (; index != umax; index--)
			{
				threads[index]->state.notify_one();
			}
		}
	} notify_threads;

	std::lock_guard lock(group->mutex);

	CellError error;

	group->run_state.fetch_op([&error](spu_group_status& state)
	{
		if (state == SPU_THREAD_GROUP_STATUS_DESTROYED)
		{
			error = CELL_ESRCH;
			return false;
		}

		// SPU_THREAD_GROUP_STATUS_READY state is not used

		if (state == SPU_THREAD_GROUP_STATUS_SUSPENDED)
		{
			state = SPU_THREAD_GROUP_STATUS_RUNNING;
		}
		else if (state == SPU_THREAD_GROUP_STATUS_WAITING_AND_SUSPENDED)
		{
			state = SPU_THREAD_GROUP_STATUS_WAITING;
			error = CellError{};
			return true;
		}
		else
		{
			error = CELL_ESTAT;
			return false;
		}

		error = CellError{CELL_CANCEL + 0u};
		return true;
	});

	if (error != CELL_CANCEL + 0u)
	{
		if (error)
		{
			return error;
		}

		return CELL_OK;
	}

	for (auto& thread : group->threads)
	{
		if (thread)
		{
			thread->state -= cpu_flag::suspend;
			notify_threads.threads[++notify_threads.index] = thread.get();
		}
	}

	return CELL_OK;
}

error_code sys_spu_thread_group_yield(ppu_thread& ppu, u32 id)
{
	ppu.state += cpu_flag::wait;

	sys_spu.trace("sys_spu_thread_group_yield(id=0x%x)", id);

	const auto group = idm::get_unlocked<lv2_spu_group>(id);

	if (!group)
	{
		return CELL_ESRCH;
	}

	// No effect on these group types
	if (!group->has_scheduler_context || group->type & 0xf00)
	{
		return CELL_OK;
	}

	if (auto state = +group->run_state; state != SPU_THREAD_GROUP_STATUS_RUNNING)
	{
		if (state == SPU_THREAD_GROUP_STATUS_DESTROYED)
		{
			return CELL_ESRCH;
		}

		return CELL_ESTAT;
	}

	// SPU_THREAD_GROUP_STATUS_READY state is not used, so this function does nothing

	return CELL_OK;
}

error_code sys_spu_thread_group_terminate(ppu_thread& ppu, u32 id, s32 value)
{
	ppu.state += cpu_flag::wait;

	sys_spu.trace("sys_spu_thread_group_terminate(id=0x%x, value=0x%x)", id, value);

	const auto group = idm::get_unlocked<lv2_spu_group>(id);

	if (!group)
	{
		return CELL_ESRCH;
	}

	std::unique_lock lock(group->mutex);

	// There should be a small period of sleep when the PPU waits for a signal of termination
	auto short_sleep = [](ppu_thread& ppu)
	{
		lv2_obj::sleep(ppu);
		busy_wait(3000);
		ppu.check_state();
		ppu.state += cpu_flag::wait;
	};

	if (auto state = +group->run_state;
		state <= SPU_THREAD_GROUP_STATUS_INITIALIZED ||
		state == SPU_THREAD_GROUP_STATUS_WAITING ||
		state == SPU_THREAD_GROUP_STATUS_WAITING_AND_SUSPENDED ||
		state == SPU_THREAD_GROUP_STATUS_DESTROYED)
	{
		if (state == SPU_THREAD_GROUP_STATUS_DESTROYED)
		{
			return CELL_ESRCH;
		}

		return CELL_ESTAT;
	}

	if (group->set_terminate)
	{
		// Wait for termination, only then return error code
		const u32 last_stop = group->stop_count;
		group->wait_term_count++;
		lock.unlock();
		short_sleep(ppu);

		while (group->stop_count == last_stop)
		{
			group->stop_count.wait(last_stop);
		}

		group->wait_term_count--;
		return CELL_ESTAT;
	}

	group->set_terminate = true;

	for (auto& thread : group->threads)
	{
		if (thread)
		{
			thread->state.fetch_op([](bs_t<cpu_flag>& flags)
			{
				if (flags & cpu_flag::stop)
				{
					// In case the thread raised the ret flag itself at some point do not raise it again
					return false;
				}

				flags += cpu_flag::stop + cpu_flag::ret;
				return true;
			});
		}
	}

	u32 prev_resv = 0;

	for (auto& thread : group->threads)
	{
		while (thread && group->running && thread->state & cpu_flag::wait)
		{
			thread_ctrl::notify(*thread);

			if (u32 resv = atomic_storage<u32>::load(thread->raddr))
			{
				if (prev_resv && prev_resv != resv)
				{
					// Batch reservation notifications if possible
					vm::reservation_notifier_notify(prev_resv);
				}

				prev_resv = resv;
			}
		}
	}

	if (prev_resv)
	{
		vm::reservation_notifier_notify(prev_resv);
	}

	group->exit_status = value;
	group->join_state = SYS_SPU_THREAD_GROUP_JOIN_TERMINATED;

	// Wait until the threads are actually stopped
	const u32 last_stop = group->stop_count;
	group->wait_term_count++;
	lock.unlock();
	short_sleep(ppu);

	while (group->stop_count == last_stop)
	{
		group->stop_count.wait(last_stop);
	}

	group->wait_term_count--;
	return CELL_OK;
}

error_code sys_spu_thread_group_join(ppu_thread& ppu, u32 id, vm::ptr<u32> cause, vm::ptr<u32> status)
{
	ppu.state += cpu_flag::wait;

	sys_spu.trace("sys_spu_thread_group_join(id=0x%x, cause=*0x%x, status=*0x%x)", id, cause, status);

	const auto group = idm::get_unlocked<lv2_spu_group>(id);

	if (!group)
	{
		return CELL_ESRCH;
	}

	do
	{
		lv2_obj::prepare_for_sleep(ppu);

		std::unique_lock lock(group->mutex);

		const auto state = +group->run_state;

		if (state == SPU_THREAD_GROUP_STATUS_DESTROYED)
		{
			return CELL_ESRCH;
		}

		if (state < SPU_THREAD_GROUP_STATUS_INITIALIZED)
		{
			return CELL_ESTAT;
		}

		if (group->waiter)
		{
			// another PPU thread is joining this thread group
			return CELL_EBUSY;
		}

		if (group->join_state && state == SPU_THREAD_GROUP_STATUS_INITIALIZED)
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
			group->waiter = &ppu;
		}

		{
			lv2_obj::notify_all_t notify;
			lv2_obj::sleep(ppu);
			lock.unlock();
		}

		while (auto state = +ppu.state)
		{
			if (state & cpu_flag::signal && ppu.state.test_and_reset(cpu_flag::signal))
			{
				break;
			}

			if (is_stopped(state))
			{
				std::lock_guard lock(group->mutex);

				if (group->waiter != &ppu)
				{
					break;
				}

				ppu.state += cpu_flag::again;
				break;
			}

			ppu.state.wait(state);
		}
	}
	while (false);

	ppu.check_state();

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
	ppu.state += cpu_flag::wait;

	sys_spu.trace("sys_spu_thread_group_set_priority(id=0x%x, priority=%d)", id, priority);

	const auto group = idm::get_unlocked<lv2_spu_group>(id);

	if (!group)
	{
		return CELL_ESRCH;
	}

	if (!group->has_scheduler_context || priority < (g_ps3_process_info.has_root_perm() ? 0 : 16) || priority > 255)
	{
		return CELL_EINVAL;
	}

	if (group->type & SYS_SPU_THREAD_GROUP_TYPE_COOPERATE_WITH_SYSTEM)
	{
		return CELL_EINVAL;
	}

	group->prio.atomic_op([&](std::common_type_t<decltype(lv2_spu_group::prio)>& prio)
	{
		prio.prio = priority;
	});

	return CELL_OK;
}

error_code sys_spu_thread_group_get_priority(ppu_thread& ppu, u32 id, vm::ptr<s32> priority)
{
	ppu.state += cpu_flag::wait;

	sys_spu.trace("sys_spu_thread_group_get_priority(id=0x%x, priority=*0x%x)", id, priority);

	const auto group = idm::get_unlocked<lv2_spu_group>(id);

	if (!group)
	{
		return CELL_ESRCH;
	}

	ppu.check_state();

	if (!group->has_scheduler_context)
	{
		*priority = 0;
	}
	else if (group->type & SYS_SPU_THREAD_GROUP_TYPE_COOPERATE_WITH_SYSTEM)
	{
		// Regardless of the value being set in group creation
		*priority = 15;
	}
	else
	{
		*priority = group->prio.load().prio;
	}

	return CELL_OK;
}

error_code sys_spu_thread_group_set_cooperative_victims(ppu_thread& ppu, u32 id, u32 threads_mask)
{
	ppu.state += cpu_flag::wait;

	sys_spu.warning("sys_spu_thread_group_set_cooperative_victims(id=0x%x, threads_mask=0x%x)", id, threads_mask);

	const auto group = idm::get_unlocked<lv2_spu_group>(id);

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
	ppu.state += cpu_flag::wait;

	sys_spu.warning("sys_spu_thread_group_syscall_253(id=0x%x, info=*0x%x)", id, info);

	const auto group = idm::get_unlocked<lv2_spu_group>(id);

	if (!group)
	{
		return CELL_ESRCH;
	}

	if (!(group->type & SYS_SPU_THREAD_GROUP_TYPE_COOPERATE_WITH_SYSTEM))
	{
		return CELL_EINVAL;
	}

	// TODO

	ppu.check_state();
	info->deadlineMissCounter = 0;
	info->deadlineMeetCounter = 0;
	info->timestamp = get_timebased_time();
	return CELL_OK;
}

error_code sys_spu_thread_write_ls(ppu_thread& ppu, u32 id, u32 lsa, u64 value, u32 type)
{
	ppu.state += cpu_flag::wait;

	sys_spu.trace("sys_spu_thread_write_ls(id=0x%x, lsa=0x%05x, value=0x%llx, type=%d)", id, lsa, value, type);

	if (lsa >= SPU_LS_SIZE || type > 8 || !type || (type | lsa) & (type - 1)) // check range and alignment
	{
		return CELL_EINVAL;
	}

	const auto [thread, group] = lv2_spu_group::get_thread(id);

	if (!thread) [[unlikely]]
	{
		return CELL_ESRCH;
	}

	std::lock_guard lock(group->mutex);

	if (auto state = +group->run_state;
		state < SPU_THREAD_GROUP_STATUS_WAITING || state > SPU_THREAD_GROUP_STATUS_RUNNING)
	{
		if (state == SPU_THREAD_GROUP_STATUS_DESTROYED)
		{
			return CELL_ESRCH;
		}

		return CELL_ESTAT;
	}

	switch (type)
	{
	case 1: thread->_ref<u8>(lsa) = static_cast<u8>(value); break;
	case 2: thread->_ref<u16>(lsa) = static_cast<u16>(value); break;
	case 4: thread->_ref<u32>(lsa) = static_cast<u32>(value); break;
	case 8: thread->_ref<u64>(lsa) = value; break;
	default: fmt::throw_exception("Unreachable");
	}

	return CELL_OK;
}

error_code sys_spu_thread_read_ls(ppu_thread& ppu, u32 id, u32 lsa, vm::ptr<u64> value, u32 type)
{
	ppu.state += cpu_flag::wait;

	sys_spu.trace("sys_spu_thread_read_ls(id=0x%x, lsa=0x%05x, value=*0x%x, type=%d)", id, lsa, value, type);

	if (lsa >= SPU_LS_SIZE || type > 8 || !type || (type | lsa) & (type - 1)) // check range and alignment
	{
		return CELL_EINVAL;
	}

	const auto [thread, group] = lv2_spu_group::get_thread(id);

	if (!thread) [[unlikely]]
	{
		return CELL_ESRCH;
	}

	std::unique_lock lock(group->mutex);

	if (auto state = +group->run_state;
		state < SPU_THREAD_GROUP_STATUS_WAITING || state > SPU_THREAD_GROUP_STATUS_RUNNING)
	{
		if (state == SPU_THREAD_GROUP_STATUS_DESTROYED)
		{
			return CELL_ESRCH;
		}

		return CELL_ESTAT;
	}

	u64 _value{};

	switch (type)
	{
	case 1: _value = thread->_ref<u8>(lsa); break;
	case 2: _value = thread->_ref<u16>(lsa); break;
	case 4: _value = thread->_ref<u32>(lsa); break;
	case 8: _value = thread->_ref<u64>(lsa); break;
	default: fmt::throw_exception("Unreachable");
	}

	lock.unlock();
	ppu.check_state();

	*value = _value;
	return CELL_OK;
}

error_code sys_spu_thread_write_spu_mb(ppu_thread& ppu, u32 id, u32 value)
{
	ppu.state += cpu_flag::wait;

	sys_spu.trace("sys_spu_thread_write_spu_mb(id=0x%x, value=0x%x)", id, value);

	const auto [thread, group] = lv2_spu_group::get_thread(id);

	if (!thread) [[unlikely]]
	{
		return CELL_ESRCH;
	}

	spu_channel_op_state state{};
	{
		std::lock_guard lock(group->mutex);

		state = thread->ch_in_mbox.push(value, true);
	}

	if (!state.op_done)
	{
		ppu.state += cpu_flag::again;
		return {};
	}

	if (state.notify)
	{
		thread->ch_in_mbox.notify();
	}

	return CELL_OK;
}

error_code sys_spu_thread_set_spu_cfg(ppu_thread& ppu, u32 id, u64 value)
{
	ppu.state += cpu_flag::wait;

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
	ppu.state += cpu_flag::wait;

	sys_spu.warning("sys_spu_thread_get_spu_cfg(id=0x%x, value=*0x%x)", id, value);

	const auto [thread, group] = lv2_spu_group::get_thread(id);

	if (!thread) [[unlikely]]
	{
		return CELL_ESRCH;
	}

	ppu.check_state();
	*value = thread->snr_config;

	return CELL_OK;
}

error_code sys_spu_thread_write_snr(ppu_thread& ppu, u32 id, u32 number, u32 value)
{
	ppu.state += cpu_flag::wait;

	sys_spu.trace("sys_spu_thread_write_snr(id=0x%x, number=%d, value=0x%x)", id, number, value);

	if (number > 1)
	{
		return CELL_EINVAL;
	}

	const auto [thread, group] = lv2_spu_group::get_thread(id);

	if (!thread) [[unlikely]]
	{
		return CELL_ESRCH;
	}

	thread->push_snr(number, value);

	return CELL_OK;
}

error_code sys_spu_thread_group_connect_event(ppu_thread& ppu, u32 id, u32 eq, u32 et)
{
	ppu.state += cpu_flag::wait;

	sys_spu.warning("sys_spu_thread_group_connect_event(id=0x%x, eq=0x%x, et=%d)", id, eq, et);

	const auto group = idm::get_unlocked<lv2_spu_group>(id);

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

	auto queue = idm::get_unlocked<lv2_obj, lv2_event_queue>(eq);

	std::lock_guard lock(group->mutex);

	if (lv2_obj::check(*ep))
	{
		return CELL_EBUSY;
	}

	// ESRCH of event queue after EBUSY
	if (!queue)
	{
		return CELL_ESRCH;
	}

	*ep = std::move(queue);
	return CELL_OK;
}

error_code sys_spu_thread_group_disconnect_event(ppu_thread& ppu, u32 id, u32 et)
{
	ppu.state += cpu_flag::wait;

	sys_spu.warning("sys_spu_thread_group_disconnect_event(id=0x%x, et=%d)", id, et);

	const auto group = idm::get_unlocked<lv2_spu_group>(id);

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
		return CELL_OK;
	}

	// No error checking is performed

	std::lock_guard lock(group->mutex);

	ep->reset();

	return CELL_OK;
}

error_code sys_spu_thread_connect_event(ppu_thread& ppu, u32 id, u32 eq, u32 et, u32 spup)
{
	ppu.state += cpu_flag::wait;

	sys_spu.warning("sys_spu_thread_connect_event(id=0x%x, eq=0x%x, et=%d, spup=%d)", id, eq, et, spup);

	const auto [thread, group] = lv2_spu_group::get_thread(id);
	auto queue = idm::get_unlocked<lv2_obj, lv2_event_queue>(eq);

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

	if (lv2_obj::check(port))
	{
		return CELL_EISCONN;
	}

	port = std::move(queue);

	return CELL_OK;
}

error_code sys_spu_thread_disconnect_event(ppu_thread& ppu, u32 id, u32 et, u32 spup)
{
	ppu.state += cpu_flag::wait;

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

	if (!lv2_obj::check(port))
	{
		return CELL_ENOTCONN;
	}

	port.reset();

	return CELL_OK;
}

error_code sys_spu_thread_bind_queue(ppu_thread& ppu, u32 id, u32 spuq, u32 spuq_num)
{
	ppu.state += cpu_flag::wait;

	sys_spu.warning("sys_spu_thread_bind_queue(id=0x%x, spuq=0x%x, spuq_num=0x%x)", id, spuq, spuq_num);

	const auto [thread, group] = lv2_spu_group::get_thread(id);
	auto queue = idm::get_unlocked<lv2_obj, lv2_event_queue>(spuq);;

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
		if (!v.second)
		{
			if (!q)
			{
				q = &v;
			}

			continue;
		}

		if (v.first == spuq_num || v.second == queue)
		{
			return CELL_EBUSY;
		}
	}

	if (!q)
	{
		return CELL_EAGAIN;
	}

	q->first = spuq_num;
	q->second = std::move(queue);
	return CELL_OK;
}

error_code sys_spu_thread_unbind_queue(ppu_thread& ppu, u32 id, u32 spuq_num)
{
	ppu.state += cpu_flag::wait;

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

		if (!v.second)
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
	ppu.state += cpu_flag::wait;

	sys_spu.warning("sys_spu_thread_group_connect_event_all_threads(id=0x%x, eq=0x%x, req=0x%llx, spup=*0x%x)", id, eq, req, spup);

	if (!req)
	{
		return CELL_EINVAL;
	}

	const auto group = idm::get_unlocked<lv2_spu_group>(id);
	const auto queue = idm::get_unlocked<lv2_obj, lv2_event_queue>(eq);

	if (!group || !queue)
	{
		return CELL_ESRCH;
	}

	std::unique_lock lock(group->mutex);

	if (auto state = +group->run_state;
		state < SPU_THREAD_GROUP_STATUS_INITIALIZED || state == SPU_THREAD_GROUP_STATUS_DESTROYED)
	{
		if (state == SPU_THREAD_GROUP_STATUS_DESTROYED)
		{
			return CELL_ESRCH;
		}

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
				if (lv2_obj::check(t->spup[port]))
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

	lock.unlock();
	ppu.check_state();

	*spup = port;

	return CELL_OK;
}

error_code sys_spu_thread_group_disconnect_event_all_threads(ppu_thread& ppu, u32 id, u32 spup)
{
	ppu.state += cpu_flag::wait;

	sys_spu.warning("sys_spu_thread_group_disconnect_event_all_threads(id=0x%x, spup=%d)", id, spup);

	if (spup > 63)
	{
		return CELL_EINVAL;
	}

	const auto group = idm::get_unlocked<lv2_spu_group>(id);

	if (!group)
	{
		return CELL_ESRCH;
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
	ppu.state += cpu_flag::wait;

	sys_spu.warning("sys_spu_thread_group_log(command=0x%x, stat=*0x%x)", command, stat);

	struct spu_group_log_state_t
	{
		atomic_t<s32> state = SYS_SPU_THREAD_GROUP_LOG_ON;
	};

	auto& state = g_fxo->get<spu_group_log_state_t>();

	switch (command)
	{
	case SYS_SPU_THREAD_GROUP_LOG_GET_STATUS:
	{
		if (!stat)
		{
			return CELL_EFAULT;
		}

		ppu.check_state();
		*stat = state.state;
		break;
	}
	case SYS_SPU_THREAD_GROUP_LOG_ON:
	case SYS_SPU_THREAD_GROUP_LOG_OFF:
	{
		state.state.release(command);
		break;
	}
	default: return CELL_EINVAL;
	}

	return CELL_OK;
}

error_code sys_spu_thread_recover_page_fault(ppu_thread& ppu, u32 id)
{
	ppu.state += cpu_flag::wait;

	sys_spu.warning("sys_spu_thread_recover_page_fault(id=0x%x)", id);

	const auto [thread, group] = lv2_spu_group::get_thread(id);

	if (!thread) [[unlikely]]
	{
		return CELL_ESRCH;
	}

	return mmapper_thread_recover_page_fault(thread);
}

error_code sys_raw_spu_recover_page_fault(ppu_thread& ppu, u32 id)
{
	ppu.state += cpu_flag::wait;

	sys_spu.warning("sys_raw_spu_recover_page_fault(id=0x%x)", id);

	const auto thread = idm::get_unlocked<named_thread<spu_thread>>(spu_thread::find_raw_spu(id));

	if (!thread) [[unlikely]]
	{
		return CELL_ESRCH;
	}

	return mmapper_thread_recover_page_fault(thread.get());
}

error_code sys_raw_spu_create(ppu_thread& ppu, vm::ptr<u32> id, vm::ptr<void> attr)
{
	ppu.state += cpu_flag::wait;

	sys_spu.warning("sys_raw_spu_create(id=*0x%x, attr=*0x%x)", id, attr);

	auto& limits = g_fxo->get<spu_limits_t>();

	std::lock_guard lock(limits.mutex);

	if (!limits.check_busy(limits_data{.raw_spu = 1}))
	{
		return CELL_EAGAIN;
	}

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

	const auto spu = idm::make_ptr<named_thread<spu_thread>>(nullptr, index, "", index);
	ensure(vm::get(vm::spu)->falloc(spu->vm_offset(), SPU_LS_SIZE, &spu->shm, vm::page_size_64k));
	spu->map_ls(*spu->shm, spu->ls);

	spu_thread::g_raw_spu_id[index] = idm::last_id();

	ppu.check_state();
	*id = index;

	return CELL_OK;
}

error_code sys_isolated_spu_create(ppu_thread& ppu, vm::ptr<u32> id, vm::ptr<void> image, u64 arg1, u64 arg2, u64 arg3, u64 arg4)
{
	ppu.state += cpu_flag::wait;

	sys_spu.todo("sys_isolated_spu_create(id=*0x%x, image=*0x%x, arg1=0x%llx, arg2=0x%llx, arg3=0x%llx, arg4=0x%llx)", id, image, arg1, arg2, arg3, arg4);

	// TODO: More accurate SPU image memory size calculation
	u32 max = image.addr() & -4096;

	while (max != 0u - 4096 && vm::check_addr(max))
	{
		max += 4096;
	}

	const auto obj = decrypt_self(fs::file{image.get_ptr(), max - image.addr()});

	if (!obj)
	{
		return CELL_EAUTHFAIL;
	}

	auto& limits = g_fxo->get<spu_limits_t>();

	std::lock_guard lock(limits.mutex);

	if (!limits.check_busy(limits_data{.raw_spu = 1}))
	{
		return CELL_EAGAIN;
	}

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

	const u32 ls_addr = RAW_SPU_BASE_ADDR + RAW_SPU_OFFSET * index;

	const auto thread = idm::make_ptr<named_thread<spu_thread>>(nullptr, index, "", index, true);

	thread->gpr[3] = v128::from64(0, arg1);
	thread->gpr[4] = v128::from64(0, arg2);
	thread->gpr[5] = v128::from64(0, arg3);
	thread->gpr[6] = v128::from64(0, arg4);

	spu_thread::g_raw_spu_id[index] = (ensure(thread->id));

	sys_spu_image img;
	img.load(obj);

	auto image_info = idm::get_unlocked<lv2_obj, lv2_spu_image>(img.entry_point);
	img.deploy(thread->ls, std::span(image_info->segs.get_ptr(), image_info->nsegs));

	thread->write_reg(ls_addr + RAW_SPU_PROB_OFFSET + SPU_NPC_offs, image_info->e_entry);
	ensure(idm::remove_verify<lv2_obj, lv2_spu_image>(img.entry_point, std::move(image_info)));

	*id = index;
	return CELL_OK;
}

template <bool isolated = false>
error_code raw_spu_destroy(ppu_thread& ppu, u32 id)
{
	const u32 idm_id = spu_thread::find_raw_spu(id);

	auto thread = idm::get<named_thread<spu_thread>>(idm_id, [](named_thread<spu_thread>& thread)
	{
		if (thread.get_type() != (isolated ? spu_type::isolated : spu_type::raw))
		{
			return false;
		}

		// Stop thread
		thread = thread_state::aborting;
		return true;
	});

	if (!thread || !thread.ret) [[unlikely]]
	{
		return CELL_ESRCH;
	}

	// TODO: CELL_EBUSY is not returned

	// Kernel objects which must be removed
	std::vector<std::pair<shared_ptr<lv2_obj>, u32>> to_remove;

	// Clear interrupt handlers
	for (auto& intr : thread->int_ctrl)
	{
		if (auto& tag = intr.tag; lv2_obj::check(tag))
		{
			if (auto& handler = tag->handler; lv2_obj::check(handler))
			{
				// SLEEP
				lv2_obj::sleep(ppu);
				handler->join();
				to_remove.emplace_back(handler, handler->id);
			}

			to_remove.emplace_back(tag, tag->id);
		}
	}

	// Remove IDs
	for (auto&& pair : to_remove)
	{
		if (pair.second >> 24 == 0xa)
			idm::remove_verify<lv2_obj, lv2_int_tag>(pair.second, std::move(pair.first));
		if (pair.second >> 24 == 0xb)
			idm::remove_verify<lv2_obj, lv2_int_serv>(pair.second, std::move(pair.first));
	}

	(*thread)();

	auto& limits = g_fxo->get<spu_limits_t>();

	std::lock_guard lock(limits.mutex);

	if (auto ret = idm::withdraw<named_thread<spu_thread>>(idm_id, [&](spu_thread& spu) -> CellError
	{
		if (std::addressof(spu) != std::addressof(*thread))
		{
			return CELL_ESRCH;
		}

		spu.cleanup();
		return {};
	}); !ret || ret.ret)
	{
		// Other thread destroyed beforehead
		return CELL_ESRCH;
	}

	return CELL_OK;
}

error_code sys_raw_spu_destroy(ppu_thread& ppu, u32 id)
{
	ppu.state += cpu_flag::wait;

	sys_spu.warning("sys_raw_spu_destroy(id=%d)", id);

	return raw_spu_destroy(ppu, id);
}

error_code sys_isolated_spu_destroy(ppu_thread& ppu, u32 id)
{
	ppu.state += cpu_flag::wait;

	sys_spu.todo("sys_isolated_spu_destroy(id=%d)", id);

	return raw_spu_destroy<true>(ppu, id);
}

template <bool isolated = false>
error_code raw_spu_create_interrupt_tag(u32 id, u32 class_id, u32 /*hwthread*/, vm::ptr<u32> intrtag)
{
	if (class_id != 0 && class_id != 2)
	{
		return CELL_EINVAL;
	}

	CellError error = {};

	const auto tag = idm::import<lv2_obj, lv2_int_tag>([&]()
	{
		shared_ptr<lv2_int_tag> result;

		auto thread = idm::check_unlocked<named_thread<spu_thread>>(spu_thread::find_raw_spu(id));

		if (!thread || *thread == thread_state::aborting || thread->get_type() != (isolated ? spu_type::isolated : spu_type::raw))
		{
			error = CELL_ESRCH;
			return result;
		}

		auto& int_ctrl = thread->int_ctrl[class_id];

		if (lv2_obj::check(int_ctrl.tag))
		{
			error = CELL_EAGAIN;
			return result;
		}

		result = make_single<lv2_int_tag>();
		int_ctrl.tag = result;
		return result;
	});

	if (tag)
	{
		cpu_thread::get_current()->check_state();
		*intrtag = tag;
		return CELL_OK;
	}

	return error;
}

error_code sys_raw_spu_create_interrupt_tag(ppu_thread& ppu, u32 id, u32 class_id, u32 hwthread, vm::ptr<u32> intrtag)
{
	ppu.state += cpu_flag::wait;

	sys_spu.warning("sys_raw_spu_create_interrupt_tag(id=%d, class_id=%d, hwthread=0x%x, intrtag=*0x%x)", id, class_id, hwthread, intrtag);

	return raw_spu_create_interrupt_tag(id, class_id, hwthread, intrtag);
}

error_code sys_isolated_spu_create_interrupt_tag(ppu_thread& ppu, u32 id, u32 class_id, u32 hwthread, vm::ptr<u32> intrtag)
{
	ppu.state += cpu_flag::wait;

	sys_spu.todo("sys_isolated_spu_create_interrupt_tag(id=%d, class_id=%d, hwthread=0x%x, intrtag=*0x%x)", id, class_id, hwthread, intrtag);

	return raw_spu_create_interrupt_tag<true>(id, class_id, hwthread, intrtag);
}

template <bool isolated = false>
error_code raw_spu_set_int_mask(u32 id, u32 class_id, u64 mask)
{
	if (class_id != 0 && class_id != 2)
	{
		return CELL_EINVAL;
	}

	const auto thread = idm::get_unlocked<named_thread<spu_thread>>(spu_thread::find_raw_spu(id));

	if (!thread || thread->get_type() != (isolated ? spu_type::isolated : spu_type::raw)) [[unlikely]]
	{
		return CELL_ESRCH;
	}

	thread->int_ctrl[class_id].mask.exchange(mask);

	return CELL_OK;
}

error_code sys_raw_spu_set_int_mask(ppu_thread& ppu, u32 id, u32 class_id, u64 mask)
{
	ppu.state += cpu_flag::wait;

	sys_spu.trace("sys_raw_spu_set_int_mask(id=%d, class_id=%d, mask=0x%llx)", id, class_id, mask);

	return raw_spu_set_int_mask(id, class_id, mask);
}


error_code sys_isolated_spu_set_int_mask(ppu_thread& ppu, u32 id, u32 class_id, u64 mask)
{
	ppu.state += cpu_flag::wait;

	sys_spu.todo("sys_isolated_spu_set_int_mask(id=%d, class_id=%d, mask=0x%llx)", id, class_id, mask);

	return raw_spu_set_int_mask<true>(id, class_id, mask);
}

template <bool isolated = false>
error_code raw_spu_set_int_stat(u32 id, u32 class_id, u64 stat)
{
	if (class_id != 0 && class_id != 2)
	{
		return CELL_EINVAL;
	}

	const auto thread = idm::get_unlocked<named_thread<spu_thread>>(spu_thread::find_raw_spu(id));

	if (!thread || thread->get_type() != (isolated ? spu_type::isolated : spu_type::raw)) [[unlikely]]
	{
		return CELL_ESRCH;
	}

	thread->int_ctrl[class_id].clear(stat);

	return CELL_OK;
}

error_code sys_raw_spu_set_int_stat(ppu_thread& ppu, u32 id, u32 class_id, u64 stat)
{
	ppu.state += cpu_flag::wait;

	sys_spu.trace("sys_raw_spu_set_int_stat(id=%d, class_id=%d, stat=0x%llx)", id, class_id, stat);

	return raw_spu_set_int_stat(id, class_id, stat);
}

error_code sys_isolated_spu_set_int_stat(ppu_thread& ppu, u32 id, u32 class_id, u64 stat)
{
	ppu.state += cpu_flag::wait;

	sys_spu.todo("sys_isolated_spu_set_int_stat(id=%d, class_id=%d, stat=0x%llx)", id, class_id, stat);

	return raw_spu_set_int_stat<true>(id, class_id, stat);
}

template <bool isolated = false>
error_code raw_spu_get_int_control(u32 id, u32 class_id, vm::ptr<u64> value, atomic_t<u64> spu_int_ctrl_t::* control)
{
	if (class_id != 0 && class_id != 2)
	{
		return CELL_EINVAL;
	}

	const auto thread = idm::get_unlocked<named_thread<spu_thread>>(spu_thread::find_raw_spu(id));

	if (!thread || thread->get_type() != (isolated ? spu_type::isolated : spu_type::raw)) [[unlikely]]
	{
		return CELL_ESRCH;
	}

	cpu_thread::get_current()->check_state();
	*value = thread->int_ctrl[class_id].*control;

	return CELL_OK;
}

error_code sys_raw_spu_get_int_mask(ppu_thread& ppu, u32 id, u32 class_id, vm::ptr<u64> mask)
{
	ppu.state += cpu_flag::wait;

	sys_spu.trace("sys_raw_spu_get_int_mask(id=%d, class_id=%d, mask=*0x%x)", id, class_id, mask);

	return raw_spu_get_int_control(id, class_id, mask, &spu_int_ctrl_t::mask);
}

error_code sys_isolated_spu_get_int_mask(ppu_thread& ppu, u32 id, u32 class_id, vm::ptr<u64> mask)
{
	ppu.state += cpu_flag::wait;

	sys_spu.trace("sys_isolated_spu_get_int_mask(id=%d, class_id=%d, mask=*0x%x)", id, class_id, mask);

	return raw_spu_get_int_control<true>(id, class_id, mask, &spu_int_ctrl_t::mask);
}

error_code sys_raw_spu_get_int_stat(ppu_thread& ppu, u32 id, u32 class_id, vm::ptr<u64> stat)
{
	ppu.state += cpu_flag::wait;

	sys_spu.trace("sys_raw_spu_get_int_stat(id=%d, class_id=%d, stat=*0x%x)", id, class_id, stat);

	return raw_spu_get_int_control(id, class_id, stat, &spu_int_ctrl_t::stat);
}

error_code sys_isolated_spu_get_int_stat(ppu_thread& ppu, u32 id, u32 class_id, vm::ptr<u64> stat)
{
	ppu.state += cpu_flag::wait;

	sys_spu.todo("sys_isolated_spu_get_int_stat(id=%d, class_id=%d, stat=*0x%x)", id, class_id, stat);

	return raw_spu_get_int_control<true>(id, class_id, stat, &spu_int_ctrl_t::stat);
}

template <bool isolated = false>
error_code raw_spu_read_puint_mb(u32 id, vm::ptr<u32> value)
{
	const auto thread = idm::get_unlocked<named_thread<spu_thread>>(spu_thread::find_raw_spu(id));

	if (!thread || thread->get_type() != (isolated ? spu_type::isolated : spu_type::raw)) [[unlikely]]
	{
		return CELL_ESRCH;
	}

	cpu_thread::get_current()->check_state();
	*value = thread->ch_out_intr_mbox.pop();

	return CELL_OK;
}

error_code sys_raw_spu_read_puint_mb(ppu_thread& ppu, u32 id, vm::ptr<u32> value)
{
	ppu.state += cpu_flag::wait;

	sys_spu.trace("sys_raw_spu_read_puint_mb(id=%d, value=*0x%x)", id, value);

	return raw_spu_read_puint_mb(id, value);
}

error_code sys_isolated_spu_read_puint_mb(ppu_thread& ppu, u32 id, vm::ptr<u32> value)
{
	ppu.state += cpu_flag::wait;

	sys_spu.todo("sys_isolated_spu_read_puint_mb(id=%d, value=*0x%x)", id, value);

	return raw_spu_read_puint_mb<true>(id, value);
}

template <bool isolated = false>
error_code raw_spu_set_spu_cfg(u32 id, u32 value)
{
	if (value > 3)
	{
		fmt::throw_exception("Unexpected value (0x%x)", value);
	}

	const auto thread = idm::get_unlocked<named_thread<spu_thread>>(spu_thread::find_raw_spu(id));

	if (!thread || thread->get_type() != (isolated ? spu_type::isolated : spu_type::raw)) [[unlikely]]
	{
		return CELL_ESRCH;
	}

	thread->snr_config = value;

	return CELL_OK;
}

error_code sys_raw_spu_set_spu_cfg(ppu_thread& ppu, u32 id, u32 value)
{
	ppu.state += cpu_flag::wait;

	sys_spu.trace("sys_raw_spu_set_spu_cfg(id=%d, value=0x%x)", id, value);

	return raw_spu_set_spu_cfg(id, value);
}

error_code sys_isolated_spu_set_spu_cfg(ppu_thread& ppu, u32 id, u32 value)
{
	ppu.state += cpu_flag::wait;

	sys_spu.todo("sys_isolated_spu_set_spu_cfg(id=%d, value=0x%x)", id, value);

	return raw_spu_set_spu_cfg<true>(id, value);
}

template <bool isolated = false>
error_code raw_spu_get_spu_cfg(u32 id, vm::ptr<u32> value)
{
	const auto thread = idm::get_unlocked<named_thread<spu_thread>>(spu_thread::find_raw_spu(id));

	if (!thread || thread->get_type() != (isolated ? spu_type::isolated : spu_type::raw)) [[unlikely]]
	{
		return CELL_ESRCH;
	}

	cpu_thread::get_current()->check_state();
	*value = static_cast<u32>(thread->snr_config);

	return CELL_OK;
}

error_code sys_raw_spu_get_spu_cfg(ppu_thread& ppu, u32 id, vm::ptr<u32> value)
{
	ppu.state += cpu_flag::wait;

	sys_spu.trace("sys_raw_spu_get_spu_afg(id=%d, value=*0x%x)", id, value);

	return raw_spu_get_spu_cfg(id, value);
}

error_code sys_isolated_spu_get_spu_cfg(ppu_thread& ppu, u32 id, vm::ptr<u32> value)
{
	ppu.state += cpu_flag::wait;

	sys_spu.todo("sys_isolated_spu_get_spu_afg(id=%d, value=*0x%x)", id, value);

	return raw_spu_get_spu_cfg<true>(id, value);
}

error_code sys_isolated_spu_start(ppu_thread& ppu, u32 id)
{
	ppu.state += cpu_flag::wait;

	sys_spu.todo("sys_isolated_spu_start(id=%d)", id);

	const auto thread = idm::get_unlocked<named_thread<spu_thread>>(spu_thread::find_raw_spu(id));

	if (!thread) [[unlikely]]
	{
		return CELL_ESRCH;
	}

	// TODO: Can return ESTAT if called twice
	thread->write_reg(RAW_SPU_BASE_ADDR + thread->lv2_id * RAW_SPU_OFFSET + RAW_SPU_PROB_OFFSET + SPU_RunCntl_offs, SPU_RUNCNTL_RUN_REQUEST);
	return CELL_OK;
}
