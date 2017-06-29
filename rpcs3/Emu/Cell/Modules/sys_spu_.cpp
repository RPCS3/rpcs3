#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/Cell/PPUModule.h"

#include "Emu/Cell/RawSPUThread.h"
#include "Emu/Cell/lv2/sys_spu.h"
#include "Crypto/unself.h"
#include "sysPrxForUser.h"

#include "Loader/ELF.h"

extern logs::channel sysPrxForUser;

extern u64 get_system_time();

spu_printf_cb_t g_spu_printf_agcb;
spu_printf_cb_t g_spu_printf_dgcb;
spu_printf_cb_t g_spu_printf_atcb;
spu_printf_cb_t g_spu_printf_dtcb;

error_code sys_spu_elf_get_information(u32 elf_img, vm::ptr<u32> entry, vm::ptr<s32> nseg)
{
	sysPrxForUser.warning("sys_spu_elf_get_information(elf_img=0x%x, entry=*0x%x, nseg=*0x%x)", elf_img, entry, nseg);

	if (!elf_img)
	{
		return CELL_EINVAL;
	}

	auto stream = fs::file{ vm::base(elf_img), size_t(-1) };
	const spu_exec_object obj{ stream };

	if (obj != elf_error::ok)
	{
		return CELL_ENOEXEC;
	}

	s32 number_segs = (s32)obj.progs.size();
	for (const auto& prog : obj.progs)
	{
		if (prog.p_type == SYS_SPU_SEGMENT_TYPE_COPY && prog.p_memsz != prog.p_filesz)
		{
			number_segs++;
		}
		else if (prog.p_type != SYS_SPU_SEGMENT_TYPE_INFO)
		{
			return CELL_ENOEXEC;
		}
	}

	*entry = obj.header.e_entry;
	*nseg = number_segs;

	return CELL_OK;
}

s32 sys_spu_elf_get_segments(u32 elf_img, vm::ptr<sys_spu_segment> segments, s32 nseg)
{
	sysPrxForUser.warning("sys_spu_elf_get_segments(elf_img=0x%x, segments=*0x%x, nseg=0x%x)", elf_img, segments, nseg);

	if (!elf_img)
	{
		return CELL_EINVAL;
	}

	auto stream = fs::file{ vm::base(elf_img), size_t(-1) };
	const spu_exec_object obj{ stream };

	if (obj != elf_error::ok)
	{
		return CELL_ENOEXEC;
	}

	int nsegs = 0;
	for (const auto& prog : obj.progs)
	{
		if (prog.p_type == SYS_SPU_SEGMENT_TYPE_COPY)
		{
			if (prog.p_filesz != 0)
			{
				auto& seg = segments[nsegs++];

				seg.type = SYS_SPU_SEGMENT_TYPE_COPY;
				seg.addr = elf_img + prog.p_offset;
				seg.ls = prog.p_vaddr;
				seg.size = prog.p_filesz;
			}

			// There's a mismatch between here and the seg counting. It seems that it exists in the real function, too.
			if (prog.p_memsz <= prog.p_filesz)
			{
				continue;
			}

			if (nsegs + 1 >= nseg)
			{
				return CELL_ENOMEM;
			}

			auto& zero = segments[nsegs++];
			zero.type = SYS_SPU_SEGMENT_TYPE_FILL;
			zero.addr = 0;
			zero.ls = prog.p_vaddr + prog.p_filesz;
			zero.size = prog.p_memsz - prog.p_filesz;
		}
	}

	// TODO some sha1 checksum or something going on here, I believe

	return CELL_OK;
}

s32 sys_spu_image_import(vm::ptr<sys_spu_image> img, u32 src, u32 type)
{
	sysPrxForUser.warning("sys_spu_image_import(img=*0x%x, src=0x%x, type=%d)", img, src, type);

	img->load(src, type);

	return CELL_OK;
}

s32 sys_spu_image_close(vm::ptr<sys_spu_image> img)
{
	sysPrxForUser.warning("sys_spu_image_close(img=*0x%x)", img);

	if (img->type == SYS_SPU_IMAGE_TYPE_USER)
	{
		//_sys_free(img->segs.addr());
	}
	else if (img->type == SYS_SPU_IMAGE_TYPE_KERNEL)
	{
		//return syscall_158(img);
	}
	else
	{
		return CELL_EINVAL;
	}

	img->free();
	return CELL_OK;
}

s32 sys_raw_spu_load(s32 id, vm::cptr<char> path, vm::ptr<u32> entry)
{
	sysPrxForUser.warning("sys_raw_spu_load(id=%d, path=%s, entry=*0x%x)", id, path, entry);

	const fs::file elf_file = fs::file(vfs::get(path.get_ptr()));

	if (elf_file)
	{
		return CELL_ENOENT;
	}

	u64 file_size = elf_file.size();
	if (!file_size)
	{
		return CELL_ENOENT;
	}
	else if (file_size > UINT_MAX)
	{
		return CELL_ENOMEM;
	}

	u32 elf_addr = vm::alloc((u32)file_size, vm::main);
	if (!elf_addr)
	{
		return CELL_ENOMEM;
	}

	u64 bytes_read = elf_file.read(vm::base(elf_addr), (u32)file_size);
	if (!bytes_read)
	{
		return CELL_ENOENT;
	}

	sys_spu_image img;
	img.load(elf_addr, SYS_SPU_IMAGE_DIRECT);
	img.deploy(RAW_SPU_BASE_ADDR + RAW_SPU_OFFSET * id);
	img.free();

	*entry = img.entry_point;

	return CELL_OK;
}

s32 sys_raw_spu_image_load(ppu_thread& ppu, s32 id, vm::ptr<sys_spu_image> img)
{
	sysPrxForUser.warning("sys_raw_spu_image_load(id=%d, img=*0x%x)", id, img);

	// Load SPU segments
	img->deploy(RAW_SPU_BASE_ADDR + RAW_SPU_OFFSET * id);

	// Use MMIO
	vm::write32(RAW_SPU_BASE_ADDR + RAW_SPU_OFFSET * id + RAW_SPU_PROB_OFFSET + SPU_NPC_offs, img->entry_point | 1);

	return CELL_OK;
}

s32 _sys_spu_printf_initialize(spu_printf_cb_t agcb, spu_printf_cb_t dgcb, spu_printf_cb_t atcb, spu_printf_cb_t dtcb)
{
	sysPrxForUser.warning("_sys_spu_printf_initialize(agcb=*0x%x, dgcb=*0x%x, atcb=*0x%x, dtcb=*0x%x)", agcb, dgcb, atcb, dtcb);

	// register callbacks
	g_spu_printf_agcb = agcb;
	g_spu_printf_dgcb = dgcb;
	g_spu_printf_atcb = atcb;
	g_spu_printf_dtcb = dtcb;

	return CELL_OK;
}

s32 _sys_spu_printf_finalize()
{
	sysPrxForUser.warning("_sys_spu_printf_finalize()");

	g_spu_printf_agcb = vm::null;
	g_spu_printf_dgcb = vm::null;
	g_spu_printf_atcb = vm::null;
	g_spu_printf_dtcb = vm::null;

	return CELL_OK;
}

s32 _sys_spu_printf_attach_group(ppu_thread& ppu, u32 group)
{
	sysPrxForUser.warning("_sys_spu_printf_attach_group(group=0x%x)", group);

	if (!g_spu_printf_agcb)
	{
		return CELL_ESTAT;
	}

	return g_spu_printf_agcb(ppu, group);
}

s32 _sys_spu_printf_detach_group(ppu_thread& ppu, u32 group)
{
	sysPrxForUser.warning("_sys_spu_printf_detach_group(group=0x%x)", group);

	if (!g_spu_printf_dgcb)
	{
		return CELL_ESTAT;
	}

	return g_spu_printf_dgcb(ppu, group);
}

s32 _sys_spu_printf_attach_thread(ppu_thread& ppu, u32 thread)
{
	sysPrxForUser.warning("_sys_spu_printf_attach_thread(thread=0x%x)", thread);

	if (!g_spu_printf_atcb)
	{
		return CELL_ESTAT;
	}

	return g_spu_printf_atcb(ppu, thread);
}

s32 _sys_spu_printf_detach_thread(ppu_thread& ppu, u32 thread)
{
	sysPrxForUser.warning("_sys_spu_printf_detach_thread(thread=0x%x)", thread);

	if (!g_spu_printf_dtcb)
	{
		return CELL_ESTAT;
	}

	return g_spu_printf_dtcb(ppu, thread);
}

void sysPrxForUser_sys_spu_init()
{
	REG_FUNC(sysPrxForUser, sys_spu_elf_get_information);
	REG_FUNC(sysPrxForUser, sys_spu_elf_get_segments);
	REG_FUNC(sysPrxForUser, sys_spu_image_import);
	REG_FUNC(sysPrxForUser, sys_spu_image_close);

	REG_FUNC(sysPrxForUser, sys_raw_spu_load);
	REG_FUNC(sysPrxForUser, sys_raw_spu_image_load);

	REG_FUNC(sysPrxForUser, _sys_spu_printf_initialize);
	REG_FUNC(sysPrxForUser, _sys_spu_printf_finalize);
	REG_FUNC(sysPrxForUser, _sys_spu_printf_attach_group);
	REG_FUNC(sysPrxForUser, _sys_spu_printf_detach_group);
	REG_FUNC(sysPrxForUser, _sys_spu_printf_attach_thread);
	REG_FUNC(sysPrxForUser, _sys_spu_printf_detach_thread);
}
