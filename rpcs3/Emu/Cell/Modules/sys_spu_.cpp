#include "stdafx.h"
#include "Emu/VFS.h"
#include "Emu/Cell/PPUModule.h"

#include "Emu/Cell/lv2/sys_spu.h"
#include "Loader/ELF.h"
#include "sysPrxForUser.h"

LOG_CHANNEL(sysPrxForUser);

spu_printf_cb_t g_spu_printf_agcb;
spu_printf_cb_t g_spu_printf_dgcb;
spu_printf_cb_t g_spu_printf_atcb;
spu_printf_cb_t g_spu_printf_dtcb;

struct spu_elf_ldr
{
	be_t<u32> _vtable;
	vm::bptr<void> src;
	be_t<u32> x8;
	be_t<u64> ehdr_off;
	be_t<u64> phdr_off;

	s32 get_ehdr(vm::ptr<elf_ehdr<elf_be, u64>> out) const
	{
		if (!src)
		{
			return -1;
		}

		if (_vtable == vm::cast(u32{1}))
		{
			vm::ptr<elf_ehdr<elf_be, u32>> ehdr = vm::cast(src.addr() + ehdr_off);
			std::memcpy(out.get_ptr(), ehdr.get_ptr(), 0x10); // Not needed?
			out->e_type      = ehdr->e_type;
			out->e_machine   = ehdr->e_machine;
			out->e_version   = ehdr->e_version;
			out->e_entry     = ehdr->e_entry;
			out->e_phoff     = ehdr->e_phoff;
			out->e_shoff     = ehdr->e_shoff;
			out->e_flags     = ehdr->e_flags;
			out->e_ehsize    = ehdr->e_ehsize;
			out->e_phentsize = ehdr->e_phentsize;
			out->e_phnum     = ehdr->e_phnum;
			out->e_shentsize = ehdr->e_shentsize;
			out->e_shnum     = ehdr->e_shnum;
			out->e_shstrndx  = ehdr->e_shstrndx;
		}
		else
		{
			vm::ptr<elf_ehdr<elf_be, u64>> ehdr = vm::cast(src.addr() + ehdr_off);
			*out = *ehdr;
		}

		return 0;
	}

	s32 get_phdr(vm::ptr<elf_phdr<elf_be, u64>> out, u32 count) const
	{
		if (!src)
		{
			return -1;
		}

		if (_vtable == vm::cast(u32{1}))
		{
			vm::ptr<elf_ehdr<elf_be, u32>> ehdr = vm::cast(src.addr() + ehdr_off);
			vm::ptr<elf_phdr<elf_be, u32>> phdr = vm::cast(src.addr() + (phdr_off ? +phdr_off : +ehdr->e_phoff));

			for (; count; count--, phdr++, out++)
			{
				out->p_type   = phdr->p_type;
				out->p_flags  = phdr->p_flags;
				out->p_offset = phdr->p_offset;
				out->p_vaddr  = phdr->p_vaddr;
				out->p_paddr  = phdr->p_paddr;
				out->p_filesz = phdr->p_filesz;
				out->p_memsz  = phdr->p_memsz;
				out->p_align  = phdr->p_align;
			}
		}
		else
		{
			vm::ptr<elf_ehdr<elf_be, u64>> ehdr = vm::cast(src.addr() + ehdr_off);
			vm::ptr<elf_phdr<elf_be, u64>> phdr = vm::cast(src.addr() + (phdr_off ? +phdr_off : +ehdr->e_phoff));

			std::memcpy(out.get_ptr(), phdr.get_ptr(), sizeof(*out) * count);
		}

		return 0;
	}
};

struct spu_elf_info
{
	u8 e_class;
	vm::bptr<spu_elf_ldr> ldr;

	struct sce_hdr
	{
		be_t<u32> se_magic;
		be_t<u32> se_hver;
		be_t<u16> se_flags;
		be_t<u16> se_type;
		be_t<u32> se_meta;
		be_t<u64> se_hsize;
		be_t<u64> se_esize;
	} sce0;

	struct self_hdr
	{
		be_t<u64> se_htype;
		be_t<u64> se_appinfooff;
		be_t<u64> se_elfoff;
		be_t<u64> se_phdroff;
		be_t<u64> se_shdroff;
		be_t<u64> se_secinfoff;
		be_t<u64> se_sceveroff;
		be_t<u64> se_controloff;
		be_t<u64> se_controlsize;
		be_t<u64> pad;
	} self;

	// Doesn't exist there
	spu_elf_ldr _overlay;

	error_code init(vm::ptr<void> src, s32 arg2 = 0)
	{
		if (!src)
		{
			return CELL_EINVAL;
		}

		u32 ehdr_off = 0;
		u32 phdr_off = 0;

		// Check SCE header if found
		std::memcpy(&sce0, src.get_ptr(), sizeof(sce0));

		if (sce0.se_magic == 0x53434500u /* SCE\0 */)
		{
			if (sce0.se_hver != 2u || sce0.se_type != 1u || sce0.se_meta == 0u)
			{
				return CELL_ENOEXEC;
			}

			std::memcpy(&self, static_cast<const uchar*>(src.get_ptr()) + sizeof(sce0), sizeof(self));
			ehdr_off = static_cast<u32>(+self.se_elfoff);
			phdr_off = static_cast<u32>(+self.se_phdroff);

			if (self.se_htype != 3u || !ehdr_off || !phdr_off)
			{
				return CELL_ENOEXEC;
			}
		}

		// Check ELF header
		vm::ptr<elf_ehdr<elf_be, u32>> ehdr = vm::cast(src.addr() + ehdr_off);

		if (ehdr->e_magic != "\177ELF"_u32 || ehdr->e_data != 2u /* BE */)
		{
			return CELL_ENOEXEC;
		}

		if (ehdr->e_class != 1u && ehdr->e_class != 2u)
		{
			return CELL_ENOEXEC;
		}

		e_class       = ehdr->e_class;
		ldr           = vm::get_addr(&_overlay);
		ldr->_vtable  = vm::cast(u32{e_class}); // TODO
		ldr->src      = vm::static_ptr_cast<u8>(src);
		ldr->x8       = arg2;
		ldr->ehdr_off = ehdr_off;
		ldr->phdr_off = phdr_off;

		return CELL_OK;
	}
};

error_code sys_spu_elf_get_information(u32 elf_img, vm::ptr<u32> entry, vm::ptr<s32> nseg)
{
	sysPrxForUser.warning("sys_spu_elf_get_information(elf_img=0x%x, entry=*0x%x, nseg=*0x%x)", elf_img, entry, nseg);

	// Initialize ELF loader
	vm::var<spu_elf_info> info({0});

	if (auto res = info->init(vm::cast(elf_img)))
	{
		return res;
	}

	// Reject SCE header
	if (info->sce0.se_magic == 0x53434500u)
	{
		return CELL_ENOEXEC;
	}

	// Load ELF header
	vm::var<elf_ehdr<elf_be, u64>> ehdr({0});

	if (info->ldr->get_ehdr(ehdr) || ehdr->e_machine != elf_machine::spu || !ehdr->e_phnum)
	{
		return CELL_ENOEXEC;
	}

	// Load program headers
	vm::var<elf_phdr<elf_be, u64>[]> phdr(ehdr->e_phnum);

	if (info->ldr->get_phdr(phdr, ehdr->e_phnum))
	{
		return CELL_ENOEXEC;
	}

	const s32 num_segs = sys_spu_image::get_nsegs<false>(phdr);

	if (num_segs < 0)
	{
		return CELL_ENOEXEC;
	}

	*entry = static_cast<u32>(ehdr->e_entry);
	*nseg  = num_segs;
	return CELL_OK;
}

error_code sys_spu_elf_get_segments(u32 elf_img, vm::ptr<sys_spu_segment> segments, s32 nseg)
{
	sysPrxForUser.warning("sys_spu_elf_get_segments(elf_img=0x%x, segments=*0x%x, nseg=0x%x)", elf_img, segments, nseg);

	// Initialize ELF loader
	vm::var<spu_elf_info> info({0});

	if (auto res = info->init(vm::cast(elf_img)))
	{
		return res;
	}

	// Load ELF header
	vm::var<elf_ehdr<elf_be, u64>> ehdr({0});

	if (info->ldr->get_ehdr(ehdr) || ehdr->e_machine != elf_machine::spu || !ehdr->e_phnum)
	{
		return CELL_ENOEXEC;
	}

	// Load program headers
	vm::var<elf_phdr<elf_be, u64>[]> phdr(ehdr->e_phnum);

	if (info->ldr->get_phdr(phdr, ehdr->e_phnum))
	{
		return CELL_ENOEXEC;
	}

	const s32 num_segs = sys_spu_image::fill<false>(segments, nseg, phdr, elf_img);

	if (num_segs == -2)
	{
		return CELL_ENOMEM;
	}
	else if (num_segs < 0)
	{
		return CELL_ENOEXEC;
	}

	return CELL_OK;
}

error_code sys_spu_image_import(ppu_thread& ppu, vm::ptr<sys_spu_image> img, u32 src, u32 type)
{
	sysPrxForUser.warning("sys_spu_image_import(img=*0x%x, src=0x%x, type=%d)", img, src, type);

	if (type != SYS_SPU_IMAGE_PROTECT && type != SYS_SPU_IMAGE_DIRECT)
	{
		return CELL_EINVAL;
	}

	// Initialize ELF loader
	vm::var<spu_elf_info> info({0});

	if (auto res = info->init(vm::cast(src)))
	{
		return res;
	}

	// Reject SCE header
	if (info->sce0.se_magic == 0x53434500u)
	{
		return CELL_ENOEXEC;
	}

	// Load ELF header
	vm::var<elf_ehdr<elf_be, u64>> ehdr({0});

	if (info->ldr->get_ehdr(ehdr) || ehdr->e_machine != elf_machine::spu || !ehdr->e_phnum)
	{
		return CELL_ENOEXEC;
	}

	// Load program headers
	vm::var<elf_phdr<elf_be, u64>[]> phdr(ehdr->e_phnum);

	if (info->ldr->get_phdr(phdr, ehdr->e_phnum))
	{
		return CELL_ENOEXEC;
	}

	if (type == SYS_SPU_IMAGE_PROTECT)
	{
		u32 img_size = 0;

		for (const auto& p : phdr)
		{
			if (p.p_type != 1u && p.p_type != 4u)
			{
				return CELL_ENOEXEC;
			}

			img_size = std::max<u32>(img_size, static_cast<u32>(p.p_offset + p.p_filesz));
		}

		return _sys_spu_image_import(ppu, img, src, img_size, 0);
	}
	else
	{
		s32 num_segs = sys_spu_image::get_nsegs(phdr);

		if (num_segs < 0)
		{
			return CELL_ENOEXEC;
		}

		img->nsegs       = num_segs;
		img->entry_point = static_cast<u32>(ehdr->e_entry);

		vm::ptr<sys_spu_segment> segs = vm::cast(vm::alloc(num_segs * sizeof(sys_spu_segment), vm::main));

		if (!segs)
		{
			return CELL_ENOMEM;
		}

		if (sys_spu_image::fill(segs, num_segs, phdr, src) != num_segs)
		{
			vm::dealloc(segs.addr());
			return CELL_ENOEXEC;
		}

		img->type = SYS_SPU_IMAGE_TYPE_USER;
		img->segs = segs;
		return CELL_OK;
	}
}

error_code sys_spu_image_close(ppu_thread& ppu, vm::ptr<sys_spu_image> img)
{
	sysPrxForUser.warning("sys_spu_image_close(img=*0x%x)", img);

	if (img->type == SYS_SPU_IMAGE_TYPE_USER)
	{
		//_sys_free(img->segs.addr());
		vm::dealloc(img->segs.addr(), vm::main);
	}
	else if (img->type == SYS_SPU_IMAGE_TYPE_KERNEL)
	{
		// Call the syscall
		return _sys_spu_image_close(ppu, img);
	}
	else
	{
		return CELL_EINVAL;
	}

	return CELL_OK;
}

error_code sys_raw_spu_load(s32 id, vm::cptr<char> path, vm::ptr<u32> entry)
{
	sysPrxForUser.warning("sys_raw_spu_load(id=%d, path=%s, entry=*0x%x)", id, path, entry);

	const fs::file elf_file = fs::file(vfs::get(path.get_ptr()));

	if (!elf_file)
	{
		return CELL_ENOENT;
	}

	sys_spu_image img;
	img.load(elf_file);
	img.deploy(vm::_ptr<u8>(RAW_SPU_BASE_ADDR + RAW_SPU_OFFSET * id), std::span(img.segs.get_ptr(), img.nsegs));
	img.free();

	*entry = img.entry_point;

	return CELL_OK;
}

error_code sys_raw_spu_image_load(s32 id, vm::ptr<sys_spu_image> img)
{
	sysPrxForUser.warning("sys_raw_spu_image_load(id=%d, img=*0x%x)", id, img);

	// Load SPU segments
	img->deploy(vm::_ptr<u8>(RAW_SPU_BASE_ADDR + RAW_SPU_OFFSET * id), std::span(img->segs.get_ptr(), img->nsegs));

	// Use MMIO
	vm::write32(RAW_SPU_BASE_ADDR + RAW_SPU_OFFSET * id + RAW_SPU_PROB_OFFSET + SPU_NPC_offs, img->entry_point);

	return CELL_OK;
}

error_code _sys_spu_printf_initialize(spu_printf_cb_t agcb, spu_printf_cb_t dgcb, spu_printf_cb_t atcb, spu_printf_cb_t dtcb)
{
	sysPrxForUser.warning("_sys_spu_printf_initialize(agcb=*0x%x, dgcb=*0x%x, atcb=*0x%x, dtcb=*0x%x)", agcb, dgcb, atcb, dtcb);

	// register callbacks
	g_spu_printf_agcb = agcb;
	g_spu_printf_dgcb = dgcb;
	g_spu_printf_atcb = atcb;
	g_spu_printf_dtcb = dtcb;

	return CELL_OK;
}

error_code _sys_spu_printf_finalize()
{
	sysPrxForUser.warning("_sys_spu_printf_finalize()");

	g_spu_printf_agcb = vm::null;
	g_spu_printf_dgcb = vm::null;
	g_spu_printf_atcb = vm::null;
	g_spu_printf_dtcb = vm::null;

	return CELL_OK;
}

error_code _sys_spu_printf_attach_group(ppu_thread& ppu, u32 group)
{
	sysPrxForUser.warning("_sys_spu_printf_attach_group(group=0x%x)", group);

	if (!g_spu_printf_agcb)
	{
		return CELL_ESTAT;
	}

	return g_spu_printf_agcb(ppu, group);
}

error_code _sys_spu_printf_detach_group(ppu_thread& ppu, u32 group)
{
	sysPrxForUser.warning("_sys_spu_printf_detach_group(group=0x%x)", group);

	if (!g_spu_printf_dgcb)
	{
		return CELL_ESTAT;
	}

	return g_spu_printf_dgcb(ppu, group);
}

error_code _sys_spu_printf_attach_thread(ppu_thread& ppu, u32 thread)
{
	sysPrxForUser.warning("_sys_spu_printf_attach_thread(thread=0x%x)", thread);

	if (!g_spu_printf_atcb)
	{
		return CELL_ESTAT;
	}

	return g_spu_printf_atcb(ppu, thread);
}

error_code _sys_spu_printf_detach_thread(ppu_thread& ppu, u32 thread)
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
