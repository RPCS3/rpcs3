#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/SysCalls/Modules.h"

#include "Emu/Cell/RawSPUThread.h"
#include "Emu/SysCalls/lv2/sys_spu.h"
#include "Emu/FS/vfsFile.h"
#include "Crypto/unself.h"
#include "sysPrxForUser.h"

extern Module sysPrxForUser;

extern u64 get_system_time();

spu_printf_cb_t g_spu_printf_agcb;
spu_printf_cb_t g_spu_printf_dgcb;
spu_printf_cb_t g_spu_printf_atcb;
spu_printf_cb_t g_spu_printf_dtcb;

s32 sys_spu_elf_get_information(u32 elf_img, vm::ptr<u32> entry, vm::ptr<s32> nseg)
{
	sysPrxForUser.Todo("sys_spu_elf_get_information(elf_img=0x%x, entry=*0x%x, nseg=*0x%x)", elf_img, entry, nseg);
	return CELL_OK;
}

s32 sys_spu_elf_get_segments(u32 elf_img, vm::ptr<sys_spu_segment> segments, s32 nseg)
{
	sysPrxForUser.Todo("sys_spu_elf_get_segments(elf_img=0x%x, segments=*0x%x, nseg=0x%x)", elf_img, segments, nseg);
	return CELL_OK;
}

s32 sys_spu_image_import(vm::ptr<sys_spu_image> img, u32 src, u32 type)
{
	sysPrxForUser.Warning("sys_spu_image_import(img=*0x%x, src=0x%x, type=%d)", img, src, type);

	return spu_image_import(*img, src, type);
}

s32 sys_spu_image_close(vm::ptr<sys_spu_image> img)
{
	sysPrxForUser.Todo("sys_spu_image_close(img=*0x%x)", img);

	return CELL_OK;
}

s32 sys_raw_spu_load(s32 id, vm::cptr<char> path, vm::ptr<u32> entry)
{
	sysPrxForUser.Warning("sys_raw_spu_load(id=%d, path=*0x%x, entry=*0x%x)", id, path, entry);
	sysPrxForUser.Warning("*** path = '%s'", path.get_ptr());

	vfsFile f(path.get_ptr());
	if (!f.IsOpened())
	{
		sysPrxForUser.Error("sys_raw_spu_load error: '%s' not found!", path.get_ptr());
		return CELL_ENOENT;
	}

	SceHeader hdr;
	hdr.Load(f);

	if (hdr.CheckMagic())
	{
		sysPrxForUser.Error("sys_raw_spu_load error: '%s' is encrypted! Decrypt SELF and try again.", path.get_ptr());
		Emu.Pause();
		return CELL_ENOENT;
	}

	f.Seek(0);

	u32 _entry;
	LoadSpuImage(f, _entry, RAW_SPU_BASE_ADDR + RAW_SPU_OFFSET * id);

	*entry = _entry | 1;

	return CELL_OK;
}

s32 sys_raw_spu_image_load(PPUThread& ppu, s32 id, vm::ptr<sys_spu_image> img)
{
	sysPrxForUser.Warning("sys_raw_spu_image_load(id=%d, img=*0x%x)", id, img);

	// TODO: use segment info

	const auto stamp0 = get_system_time();

	memcpy(vm::get_ptr<void>(RAW_SPU_BASE_ADDR + RAW_SPU_OFFSET * id), vm::get_ptr<void>(img->addr), 256 * 1024);

	const auto stamp1 = get_system_time();

	vm::write32(RAW_SPU_BASE_ADDR + RAW_SPU_OFFSET * id + RAW_SPU_PROB_OFFSET + SPU_NPC_offs, img->entry_point | 1);

	const auto stamp2 = get_system_time();

	sysPrxForUser.Error("memcpy() latency: %lldus", (stamp1 - stamp0));
	sysPrxForUser.Error("MMIO latency: %lldus", (stamp2 - stamp1));

	return CELL_OK;
}

s32 _sys_spu_printf_initialize(spu_printf_cb_t agcb, spu_printf_cb_t dgcb, spu_printf_cb_t atcb, spu_printf_cb_t dtcb)
{
	sysPrxForUser.Warning("_sys_spu_printf_initialize(agcb=*0x%x, dgcb=*0x%x, atcb=*0x%x, dtcb=*0x%x)", agcb, dgcb, atcb, dtcb);

	// register callbacks
	g_spu_printf_agcb = agcb;
	g_spu_printf_dgcb = dgcb;
	g_spu_printf_atcb = atcb;
	g_spu_printf_dtcb = dtcb;

	return CELL_OK;
}

s32 _sys_spu_printf_finalize()
{
	sysPrxForUser.Warning("_sys_spu_printf_finalize()");

	g_spu_printf_agcb = vm::null;
	g_spu_printf_dgcb = vm::null;
	g_spu_printf_atcb = vm::null;
	g_spu_printf_dtcb = vm::null;

	return CELL_OK;
}

s32 _sys_spu_printf_attach_group(PPUThread& ppu, u32 group)
{
	sysPrxForUser.Warning("_sys_spu_printf_attach_group(group=0x%x)", group);

	if (!g_spu_printf_agcb)
	{
		return CELL_ESTAT;
	}

	return g_spu_printf_agcb(ppu, group);
}

s32 _sys_spu_printf_detach_group(PPUThread& ppu, u32 group)
{
	sysPrxForUser.Warning("_sys_spu_printf_detach_group(group=0x%x)", group);

	if (!g_spu_printf_dgcb)
	{
		return CELL_ESTAT;
	}

	return g_spu_printf_dgcb(ppu, group);
}

s32 _sys_spu_printf_attach_thread(PPUThread& ppu, u32 thread)
{
	sysPrxForUser.Warning("_sys_spu_printf_attach_thread(thread=0x%x)", thread);

	if (!g_spu_printf_atcb)
	{
		return CELL_ESTAT;
	}

	return g_spu_printf_atcb(ppu, thread);
}

s32 _sys_spu_printf_detach_thread(PPUThread& ppu, u32 thread)
{
	sysPrxForUser.Warning("_sys_spu_printf_detach_thread(thread=0x%x)", thread);

	if (!g_spu_printf_dtcb)
	{
		return CELL_ESTAT;
	}

	return g_spu_printf_dtcb(ppu, thread);
}

void sysPrxForUser_sys_spu_init()
{
	g_spu_printf_agcb = vm::null;
	g_spu_printf_dgcb = vm::null;
	g_spu_printf_atcb = vm::null;
	g_spu_printf_dtcb = vm::null;

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
