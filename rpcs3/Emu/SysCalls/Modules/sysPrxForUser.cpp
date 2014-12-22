#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/SysCalls/Modules.h"
#include "Emu/SysCalls/CB_FUNC.h"
#include "Emu/Memory/atomic_type.h"
#include "Utilities/SMutex.h"

#include "Emu/FS/vfsFile.h"
#include "Emu/SysCalls/lv2/sys_spu.h"
#include "Emu/SysCalls/lv2/sys_lwmutex.h"
#include "Emu/SysCalls/lv2/sys_spinlock.h"
#include "Emu/SysCalls/lv2/sys_prx.h"
#include "Emu/SysCalls/lv2/sys_ppu_thread.h"
#include "Emu/SysCalls/lv2/sys_process.h"
#include "Emu/SysCalls/lv2/sys_time.h"
#include "Emu/SysCalls/lv2/sys_mmapper.h"
#include "Emu/SysCalls/lv2/sys_lwcond.h"
#include "Loader/ELF32.h"
#include "Crypto/unself.h"
#include "Emu/Cell/RawSPUThread.h"
#include "sysPrxForUser.h"

Module *sysPrxForUser = nullptr;

void sys_initialize_tls()
{
	sysPrxForUser->Log("sys_initialize_tls()");
}

int _sys_heap_create_heap(const u32 heap_addr, const u32 align, const u32 size)
{
	sysPrxForUser->Warning("_sys_heap_create_heap(heap_addr=0x%x, align=0x%x, size=0x%x)", heap_addr, align, size);

	u32 heap_id = sysPrxForUser->GetNewId(new HeapInfo(heap_addr, align, size));
	sysPrxForUser->Warning("*** sys_heap created: id = %d", heap_id);
	return heap_id;
}

u32 _sys_heap_malloc(const u32 heap_id, const u32 size)
{
	sysPrxForUser->Warning("_sys_heap_malloc(heap_id=%d, size=0x%x)", heap_id, size);

	HeapInfo* heap;
	if(!sysPrxForUser->CheckId(heap_id, heap)) return CELL_ESRCH;

	return (u32)Memory.Alloc(size, 1);
}

u32 _sys_heap_memalign(u32 heap_id, u32 align, u32 size)
{
	sysPrxForUser->Warning("_sys_heap_memalign(heap_id=%d, align=0x%x, size=0x%x)", heap_id, align, size);

	HeapInfo* heap;
	if(!sysPrxForUser->CheckId(heap_id, heap)) return CELL_ESRCH;

	return (u32)Memory.Alloc(size, align);
}

s64 _sys_process_atexitspawn()
{
	sysPrxForUser->Log("_sys_process_atexitspawn()");
	return CELL_OK;
}

s64 _sys_process_at_Exitspawn()
{
	sysPrxForUser->Log("_sys_process_at_Exitspawn");
	return CELL_OK;
}

int sys_process_is_stack(u32 p)
{
	sysPrxForUser->Log("sys_process_is_stack(p=0x%x)", p);

	// prx: compare high 4 bits with "0xD"
	return (p >= Memory.StackMem.GetStartAddr() && p <= Memory.StackMem.GetEndAddr()) ? 1 : 0;
}

s64 sys_prx_exitspawn_with_level()
{
	sysPrxForUser->Log("sys_prx_exitspawn_with_level()");
	return CELL_OK;
}

int sys_spu_elf_get_information(u32 elf_img, vm::ptr<u32> entry, vm::ptr<u32> nseg)
{
	sysPrxForUser->Todo("sys_spu_elf_get_information(elf_img=0x%x, entry_addr=0x%x, nseg_addr=0x%x)", elf_img, entry.addr(), nseg.addr());
	return CELL_OK;
}

int sys_spu_elf_get_segments(u32 elf_img, vm::ptr<sys_spu_segment> segments, int nseg)
{
	sysPrxForUser->Todo("sys_spu_elf_get_segments(elf_img=0x%x, segments_addr=0x%x, nseg=0x%x)", elf_img, segments.addr(), nseg);
	return CELL_OK;
}

int sys_spu_image_import(vm::ptr<sys_spu_image> img, u32 src, u32 type)
{
	sysPrxForUser->Warning("sys_spu_image_import(img=0x%x, src=0x%x, type=%d)", img.addr(), src, type);

	return spu_image_import(*img, src, type);
}

int sys_spu_image_close(vm::ptr<sys_spu_image> img)
{
	sysPrxForUser->Warning("sys_spu_image_close(img=0x%x)", img.addr());
	return CELL_OK;
}

int sys_raw_spu_load(s32 id, vm::ptr<const char> path, vm::ptr<u32> entry)
{
	sysPrxForUser->Warning("sys_raw_spu_load(id=0x%x, path_addr=0x%x('%s'), entry_addr=0x%x)", 
		id, path.addr(), path.get_ptr(), entry.addr());

	vfsFile f(path.get_ptr());
	if(!f.IsOpened())
	{
		sysPrxForUser->Error("sys_raw_spu_load error: '%s' not found!", path.get_ptr());
		return CELL_ENOENT;
	}

	SceHeader hdr;
	hdr.Load(f);

	if (hdr.CheckMagic())
	{
		sysPrxForUser->Error("sys_raw_spu_load error: '%s' is encrypted! Decrypt SELF and try again.", path.get_ptr());
		Emu.Pause();
		return CELL_ENOENT;
	}

	f.Seek(0);

	u32 _entry;
	LoadSpuImage(f, _entry, RAW_SPU_BASE_ADDR + RAW_SPU_OFFSET * id);

	*entry = _entry;

	return CELL_OK;
}

int sys_raw_spu_image_load(int id, vm::ptr<sys_spu_image> img)
{
	sysPrxForUser->Warning("sys_raw_spu_image_load(id=0x%x, img_addr=0x%x)", id, img.addr());

	// TODO: use segment info
	memcpy(vm::get_ptr<void>(RAW_SPU_BASE_ADDR + RAW_SPU_OFFSET * id), vm::get_ptr<void>(img->addr), 256 * 1024);
	vm::write32(RAW_SPU_BASE_ADDR + RAW_SPU_OFFSET * id + RAW_SPU_PROB_OFFSET + SPU_NPC_offs, (u32)img->entry_point);

	return CELL_OK;
}

int sys_get_random_number(vm::ptr<u8> addr, u64 size)
{
	sysPrxForUser->Warning("sys_get_random_number(addr=0x%x, size=%d)", addr.addr(), size);

	if (size > 4096)
		size = 4096;

	for (u32 i = 0; i < (u32)size - 1; i++)
	{
		addr[i] = rand() % 256;
	}

	return CELL_OK;
}

vm::ptr<void> _sys_memset(vm::ptr<void> dst, s32 value, u32 size)
{
	sysPrxForUser->Log("_sys_memset(dst_addr=0x%x, value=%d, size=%d)", dst.addr(), value, size);

	memset(dst.get_ptr(), value, size);
	return dst;
}

vm::ptr<void> _sys_memcpy(vm::ptr<void> dst, vm::ptr<const void> src, u32 size)
{
	sysPrxForUser->Log("_sys_memcpy(dst_addr=0x%x, src_addr=0x%x, size=%d)", dst.addr(), src.addr(), size);

	memcpy(dst.get_ptr(), src.get_ptr(), size);
	return dst;
}

s32 _sys_memcmp(vm::ptr<const void> buf1, vm::ptr<const void> buf2, u32 size)
{
	sysPrxForUser->Log("_sys_memcmp(buf1_addr=0x%x, buf2_addr=0x%x, size=%d)", buf1.addr(), buf2.addr(), size);

	return memcmp(buf1.get_ptr(), buf2.get_ptr(), size);
}

s64 _sys_strlen(vm::ptr<const char> str)
{
	sysPrxForUser->Log("_sys_strlen(str_addr=0x%x)", str.addr());

	return strlen(str.get_ptr());
}

s32 _sys_strncmp(vm::ptr<const char> str1, vm::ptr<const char> str2, s32 max)
{
	sysPrxForUser->Log("_sys_strncmp(str1_addr=0x%x, str2_addr=0x%x, max=%d)", str1.addr(), str2.addr(), max);

	return strncmp(str1.get_ptr(), str2.get_ptr(), max);
}

vm::ptr<char> _sys_strcat(vm::ptr<char> dest, vm::ptr<const char> source)
{
	sysPrxForUser->Log("_sys_strcat(dest_addr=0x%x, source_addr=0x%x)", dest.addr(), source.addr());

	if (strcat(dest.get_ptr(), source.get_ptr()) != dest.get_ptr())
	{
		assert(!"strcat(): unexpected result");
	}
	return dest;
}

vm::ptr<char> _sys_strncat(vm::ptr<char> dest, vm::ptr<const char> source, u32 len)
{
	sysPrxForUser->Log("_sys_strncat(dest_addr=0x%x, source_addr=0x%x, len=%d)", dest.addr(), source.addr(), len);

	if (strncat(dest.get_ptr(), source.get_ptr(), len) != dest.get_ptr())
	{
		assert(!"strncat(): unexpected result");
	}
	return dest;
}

vm::ptr<char> _sys_strcpy(vm::ptr<char> dest, vm::ptr<const char> source)
{
	sysPrxForUser->Log("_sys_strcpy(dest_addr=0x%x, source_addr=0x%x)", dest.addr(), source.addr());

	if (strcpy(dest.get_ptr(), source.get_ptr()) != dest.get_ptr())
	{
		assert(!"strcpy(): unexpected result");
	}
	return dest;
}

vm::ptr<char> _sys_strncpy(vm::ptr<char> dest, vm::ptr<const char> source, u32 len)
{
	sysPrxForUser->Log("_sys_strncpy(dest_addr=0x%x, source_addr=0x%x, len=%d)", dest.addr(), source.addr(), len);

	if (!dest || !source)
	{
		return vm::ptr<char>::make(0);
	}

	if (strncpy(dest.get_ptr(), source.get_ptr(), len) != dest.get_ptr())
	{
		assert(!"strncpy(): unexpected result");
	}
	return dest;
}

vm::ptr<spu_printf_cb_t> spu_printf_agcb;
vm::ptr<spu_printf_cb_t> spu_printf_dgcb;
vm::ptr<spu_printf_cb_t> spu_printf_atcb;
vm::ptr<spu_printf_cb_t> spu_printf_dtcb;

s32 _sys_spu_printf_initialize(
	vm::ptr<spu_printf_cb_t> agcb,
	vm::ptr<spu_printf_cb_t> dgcb,
	vm::ptr<spu_printf_cb_t> atcb,
	vm::ptr<spu_printf_cb_t> dtcb)
{
	sysPrxForUser->Warning("_sys_spu_printf_initialize(agcb_addr=0x%x, dgcb_addr=0x%x, atcb_addr=0x%x, dtcb_addr=0x%x)",
		agcb.addr(), dgcb.addr(), atcb.addr(), dtcb.addr());

	// prx: register some callbacks
	spu_printf_agcb = agcb;
	spu_printf_dgcb = dgcb;
	spu_printf_atcb = atcb;
	spu_printf_dtcb = dtcb;
	return CELL_OK;
}

s32 _sys_spu_printf_finalize()
{
	sysPrxForUser->Warning("_sys_spu_printf_finalize()");

	spu_printf_agcb.set(0);
	spu_printf_dgcb.set(0);
	spu_printf_atcb.set(0);
	spu_printf_dtcb.set(0);
	return CELL_OK;
}

s64 _sys_spu_printf_attach_group(u32 group)
{
	sysPrxForUser->Warning("_sys_spu_printf_attach_group(group=%d)", group);

	if (!spu_printf_agcb)
	{
		return CELL_ESTAT;
	}

	return spu_printf_agcb(group);
}

s64 _sys_spu_printf_detach_group(u32 group)
{
	sysPrxForUser->Warning("_sys_spu_printf_detach_group(group=%d)", group);

	if (!spu_printf_dgcb)
	{
		return CELL_ESTAT;
	}

	return spu_printf_dgcb(group);
}

s64 _sys_spu_printf_attach_thread(u32 thread)
{
	sysPrxForUser->Warning("_sys_spu_printf_attach_thread(thread=%d)", thread);

	if (!spu_printf_atcb)
	{
		return CELL_ESTAT;
	}

	return spu_printf_atcb(thread);
}

s64 _sys_spu_printf_detach_thread(u32 thread)
{
	sysPrxForUser->Warning("_sys_spu_printf_detach_thread(thread=%d)", thread);

	if (!spu_printf_dtcb)
	{
		return CELL_ESTAT;
	}

	return spu_printf_dtcb(thread);
}

s32 _sys_snprintf(vm::ptr<char> dst, u32 count, vm::ptr<const char> fmt) // va_args...
{
	sysPrxForUser->Todo("_sys_snprintf(dst_addr=0x%x, count=%d, fmt_addr=0x%x['%s'], ...)", dst.addr(), count, fmt.addr(), fmt.get_ptr());

	Emu.Pause();
	return 0;
}

s32 _sys_printf(vm::ptr<const char> fmt) // va_args...
{
	sysPrxForUser->Todo("_sys_printf(fmt_addr=0x%x, ...)", fmt.addr());

	// probably, assertion failed
	sysPrxForUser->Warning("_sys_printf: \n%s", fmt.get_ptr());
	Emu.Pause();
	return CELL_OK;
}

s32 _unnamed_E75C40F2(u32 dest)
{
	sysPrxForUser->Todo("Unnamed function 0xE75C40F2 (dest=0x%x) -> CELL_ENOENT", dest);

	// prx: load some data (0x40 bytes) previously set by sys_process_get_paramsfo
	//memset(Memory + dest, 0, 0x40);
	return CELL_ENOENT;
}

void sysPrxForUser_init(Module *pxThis)
{
	sysPrxForUser = pxThis;

	// Setup random number generator
	srand(time(NULL));

	REG_FUNC(sysPrxForUser, sys_initialize_tls);
	
	REG_FUNC(sysPrxForUser, sys_lwmutex_create);
	REG_FUNC(sysPrxForUser, sys_lwmutex_destroy);
	REG_FUNC(sysPrxForUser, sys_lwmutex_lock);
	REG_FUNC(sysPrxForUser, sys_lwmutex_trylock);
	REG_FUNC(sysPrxForUser, sys_lwmutex_unlock);

	sysPrxForUser->AddFunc(0x8461e528, sys_time_get_system_time);

	sysPrxForUser->AddFunc(0xe6f2c1e7, sys_process_exit);
	sysPrxForUser->AddFunc(0x2c847572, _sys_process_atexitspawn);
	sysPrxForUser->AddFunc(0x96328741, _sys_process_at_Exitspawn);
	sysPrxForUser->AddFunc(0x4f7172c9, sys_process_is_stack);

	sysPrxForUser->AddFunc(0x24a1ea07, sys_ppu_thread_create);
	sysPrxForUser->AddFunc(0x350d454e, sys_ppu_thread_get_id);
	sysPrxForUser->AddFunc(0xaff080a4, sys_ppu_thread_exit);
	sysPrxForUser->AddFunc(0xa3e3be68, sys_ppu_thread_once);

	sysPrxForUser->AddFunc(0x26090058, sys_prx_load_module);
	sysPrxForUser->AddFunc(0x9f18429d, sys_prx_start_module);
	sysPrxForUser->AddFunc(0x80fb0c19, sys_prx_stop_module);
	sysPrxForUser->AddFunc(0xf0aece0d, sys_prx_unload_module);
	sysPrxForUser->AddFunc(0x42b23552, sys_prx_register_library);
	sysPrxForUser->AddFunc(0xd0ea47a7, sys_prx_unregister_library);
	sysPrxForUser->AddFunc(0xa5d06bf0, sys_prx_get_module_list);
	sysPrxForUser->AddFunc(0x84bb6774, sys_prx_get_module_info);
	sysPrxForUser->AddFunc(0xe0998dbf, sys_prx_get_module_id_by_name);
	sysPrxForUser->AddFunc(0xaa6d9bff, sys_prx_load_module_on_memcontainer);
	sysPrxForUser->AddFunc(0xa2c7ba64, sys_prx_exitspawn_with_level);

	sysPrxForUser->AddFunc(0x35168520, _sys_heap_malloc);
	//sysPrxForUser->AddFunc(0xaede4b03, _sys_heap_free);
	//sysPrxForUser->AddFunc(0x8a561d92, _sys_heap_delete_heap);
	sysPrxForUser->AddFunc(0xb2fcf2c8, _sys_heap_create_heap);
	sysPrxForUser->AddFunc(0x44265c08, _sys_heap_memalign);

	sysPrxForUser->AddFunc(0xb257540b, sys_mmapper_allocate_memory);
	sysPrxForUser->AddFunc(0x70258515, sys_mmapper_allocate_memory_from_container);
	sysPrxForUser->AddFunc(0xdc578057, sys_mmapper_map_memory);
	sysPrxForUser->AddFunc(0x4643ba6e, sys_mmapper_unmap_memory);
	sysPrxForUser->AddFunc(0x409ad939, sys_mmapper_free_memory);

	sysPrxForUser->AddFunc(0x1ed454ce, sys_spu_elf_get_information);
	sysPrxForUser->AddFunc(0xdb6b3250, sys_spu_elf_get_segments);
	sysPrxForUser->AddFunc(0xebe5f72f, sys_spu_image_import);
	sysPrxForUser->AddFunc(0xe0da8efd, sys_spu_image_close);

	sysPrxForUser->AddFunc(0x893305fa, sys_raw_spu_load);
	sysPrxForUser->AddFunc(0xb995662e, sys_raw_spu_image_load);

	sysPrxForUser->AddFunc(0xda0eb71a, sys_lwcond_create);
	sysPrxForUser->AddFunc(0x1c9a942c, sys_lwcond_destroy);
	sysPrxForUser->AddFunc(0xef87a695, sys_lwcond_signal);
	sysPrxForUser->AddFunc(0xe9a1bd84, sys_lwcond_signal_all);
	sysPrxForUser->AddFunc(0x52aadadf, sys_lwcond_signal_to);
	sysPrxForUser->AddFunc(0x2a6d9d51, sys_lwcond_wait);

	sysPrxForUser->AddFunc(0x71a8472a, sys_get_random_number);

	sysPrxForUser->AddFunc(0x8c2bb498, sys_spinlock_initialize);
	sysPrxForUser->AddFunc(0xa285139d, sys_spinlock_lock);
	sysPrxForUser->AddFunc(0x722a0254, sys_spinlock_trylock);
	sysPrxForUser->AddFunc(0x5267cb35, sys_spinlock_unlock);

	sysPrxForUser->AddFunc(0x67f9fedb, sys_game_process_exitspawn2);
	sysPrxForUser->AddFunc(0xfc52a7a9, sys_game_process_exitspawn);

	REG_FUNC(sysPrxForUser, _sys_memset);
	REG_FUNC(sysPrxForUser, _sys_memcpy);
	REG_FUNC(sysPrxForUser, _sys_memcmp);
	REG_FUNC(sysPrxForUser, _sys_strlen);
	REG_FUNC(sysPrxForUser, _sys_strncmp);
	REG_FUNC(sysPrxForUser, _sys_strcat);
	REG_FUNC(sysPrxForUser, _sys_strncat);
	REG_FUNC(sysPrxForUser, _sys_strcpy);
	REG_FUNC(sysPrxForUser, _sys_strncpy);

	REG_FUNC(sysPrxForUser, _sys_spu_printf_initialize);
	REG_FUNC(sysPrxForUser, _sys_spu_printf_finalize);
	REG_FUNC(sysPrxForUser, _sys_spu_printf_attach_group);
	REG_FUNC(sysPrxForUser, _sys_spu_printf_detach_group);
	REG_FUNC(sysPrxForUser, _sys_spu_printf_attach_thread);
	REG_FUNC(sysPrxForUser, _sys_spu_printf_detach_thread);

	REG_FUNC(sysPrxForUser, _sys_snprintf);

	REG_FUNC(sysPrxForUser, _sys_printf);
	sysPrxForUser->AddFunc(0xe75c40f2, _unnamed_E75C40F2); // real name is unknown
}

void sysPrxForUser_load()
{
	spu_printf_agcb.set(0);
	spu_printf_dgcb.set(0);
	spu_printf_atcb.set(0);
	spu_printf_dtcb.set(0);
}