#include "stdafx.h"
#include "Utilities/Log.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/Cell/PPUThread.h"
#include "Emu/SysCalls/SC_FUNC.h"
#include "Emu/SysCalls/Modules.h"
#include "Emu/FS/vfsFile.h"
#include "Emu/FS/vfsStreamMemory.h"
#include "Emu/SysCalls/lv2/sys_spu.h"
#include "Loader/ELF.h"
#include "Emu/Cell/RawSPUThread.h"
#include "sysPrxForUser.h"

//void sysPrxForUser_init();
//Module sysPrxForUser("sysPrxForUser", sysPrxForUser_init);
Module *sysPrxForUser = nullptr;

int sys_heap_create_heap(const u32 heap_addr, const u32 align, const u32 size)
{	
	sysPrxForUser->Warning("sys_heap_create_heap(heap_addr=0x%x, align=0x%x, size=0x%x)", heap_addr, align, size);

	u32 heap_id = sysPrxForUser->GetNewId(new HeapInfo(heap_addr, align, size));
	sysPrxForUser->Warning("*** sys_heap created: id = %d", heap_id);
	return heap_id;
}

int sys_heap_malloc(const u32 heap_id, const u32 size)
{
	sysPrxForUser->Warning("sys_heap_malloc(heap_id=%d, size=0x%x)", heap_id, size);

	HeapInfo* heap;
	if(!sysPrxForUser->CheckId(heap_id, heap)) return CELL_ESRCH;

	return Memory.Alloc(size, 1);
}

int _sys_heap_memalign(u32 heap_id, u32 align, u32 size)
{
	sysPrxForUser->Warning("_sys_heap_memalign(heap_id=%d, align=0x%x, size=0x%x)", heap_id, align, size);

	HeapInfo* heap;
	if(!sysPrxForUser->CheckId(heap_id, heap)) return CELL_ESRCH;

	return Memory.Alloc(size, align);
}

void sys_initialize_tls()
{
	sysPrxForUser->Log("sys_initialize_tls()");
}

s64 sys_process_atexitspawn()
{
	sysPrxForUser->Log("sys_process_atexitspawn()");
	return CELL_OK;
}

s64 sys_process_at_Exitspawn()
{
	sysPrxForUser->Log("sys_process_at_Exitspawn");
	return CELL_OK;
}

int sys_process_is_stack(u32 p)
{
	sysPrxForUser->Log("sys_process_is_stack(p=0x%x)", p);

	// prx: compare high 4 bits with "0xD"
	return (int)(bool)(p >= Memory.StackMem.GetStartAddr() && p <= Memory.StackMem.GetEndAddr());
}

int sys_spu_printf_initialize(int a1, int a2, int a3, int a4, int a5)
{
	sysPrxForUser->Warning("sys_spu_printf_initialize(0x%x, 0x%x, 0x%x, 0x%x, 0x%x)", a1, a2, a3, a4, a5);
	return CELL_OK;
}

s64 sys_prx_exitspawn_with_level()
{
	sysPrxForUser->Log("sys_prx_exitspawn_with_level()");
	return CELL_OK;
}

s64 sys_strlen(u32 addr)
{
	const std::string& str = Memory.ReadString(addr);
	sysPrxForUser->Log("sys_strlen(0x%x - \"%s\")", addr, str.c_str());
	return str.length();
}

int sys_spu_elf_get_information(u32 elf_img, mem32_t entry, mem32_t nseg)
{
	sysPrxForUser->Warning("sys_spu_elf_get_information(elf_img=0x%x, entry_addr=0x%x, nseg_addr=0x%x", elf_img, entry.GetAddr(), nseg.GetAddr());
	return CELL_OK;
}

int sys_spu_elf_get_segments(u32 elf_img, mem_ptr_t<sys_spu_segment> segments, int nseg)
{
	sysPrxForUser->Warning("sys_spu_elf_get_segments(elf_img=0x%x, segments_addr=0x%x, nseg=0x%x)", elf_img, segments.GetAddr(), nseg);
	return CELL_OK;
}

int sys_spu_image_import(mem_ptr_t<sys_spu_image> img, u32 src, u32 type)
{
	sysPrxForUser->Warning("sys_spu_image_import(img=0x%x, src=0x%x, type=0x%x)", img.GetAddr(), src, type);

	vfsStreamMemory f(src);
	u32 entry;
	u32 offset = LoadSpuImage(f, entry);

	img->type = type;
	img->entry_point = entry;
	img->segs_addr = offset;
	img->nsegs = 0;

	return CELL_OK;
}

int sys_spu_image_close(mem_ptr_t<sys_spu_image> img)
{
	sysPrxForUser->Warning("sys_spu_image_close(img=0x%x)", img.GetAddr());
	return CELL_OK;
}

int sys_raw_spu_load(int id, u32 path_addr, mem32_t entry)
{
	const std::string path = Memory.ReadString(path_addr);
	sysPrxForUser->Warning("sys_raw_spu_load(id=0x%x, path=0x%x [%s], entry_addr=0x%x)", 
		id, path_addr, path.c_str(), entry.GetAddr());

	vfsFile f(path);
	if(!f.IsOpened())
	{
		sysPrxForUser->Error("sys_raw_spu_load error: '%s' not found!", path.c_str());
		return CELL_ENOENT;
	}

	ELFLoader l(f);
	l.LoadInfo();
	l.LoadData(RAW_SPU_BASE_ADDR + RAW_SPU_OFFSET * id);

	entry = l.GetEntry();

	return CELL_OK;
}

int sys_raw_spu_image_load(int id, mem_ptr_t<sys_spu_image> img)
{
	sysPrxForUser->Warning("sys_raw_spu_image_load(id=0x%x, img_addr=0x%x)", id, img.GetAddr());

	if (!Memory.Copy(RAW_SPU_BASE_ADDR + RAW_SPU_OFFSET * id, (u32)img->segs_addr, 256 * 1024))
	{
		sysPrxForUser->Error("sys_raw_spu_image_load() failed");
		return CELL_EFAULT;
	}
	Memory.Write32(RAW_SPU_BASE_ADDR + RAW_SPU_OFFSET * id + RAW_SPU_PROB_OFFSET + SPU_NPC_offs, (u32)img->entry_point);

	return CELL_OK;
}

s32 _sys_memset(u32 addr, s32 value, u32 size)
{
	sysPrxForUser->Log("_sys_memset(addr=0x%x, value=%d, size=%d)", addr, value, size);

	memset(Memory + addr, value, size);
	return CELL_OK;
}

void sysPrxForUser_init()
{
	sysPrxForUser->AddFunc(0x744680a2, sys_initialize_tls);
	
	sysPrxForUser->AddFunc(0x2f85c0ef, sys_lwmutex_create);
	sysPrxForUser->AddFunc(0xc3476d0c, sys_lwmutex_destroy);
	sysPrxForUser->AddFunc(0x1573dc3f, sys_lwmutex_lock);
	sysPrxForUser->AddFunc(0xaeb78725, sys_lwmutex_trylock);
	sysPrxForUser->AddFunc(0x1bc200f4, sys_lwmutex_unlock);

	sysPrxForUser->AddFunc(0x8461e528, sys_time_get_system_time);

	sysPrxForUser->AddFunc(0xe6f2c1e7, sys_process_exit);
	sysPrxForUser->AddFunc(0x2c847572, sys_process_atexitspawn);
	sysPrxForUser->AddFunc(0x96328741, sys_process_at_Exitspawn);
	sysPrxForUser->AddFunc(0x4f7172c9, sys_process_is_stack);

	sysPrxForUser->AddFunc(0x24a1ea07, sys_ppu_thread_create);
	sysPrxForUser->AddFunc(0x350d454e, sys_ppu_thread_get_id);
	sysPrxForUser->AddFunc(0xaff080a4, sys_ppu_thread_exit);
	sysPrxForUser->AddFunc(0xa3e3be68, sys_ppu_thread_once);

	sysPrxForUser->AddFunc(0x45fe2fce, sys_spu_printf_initialize);

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

	sysPrxForUser->AddFunc(0x2d36462b, sys_strlen);

	sysPrxForUser->AddFunc(0x35168520, sys_heap_malloc);
	//sysPrxForUser->AddFunc(0xaede4b03, sys_heap_free);
	//sysPrxForUser->AddFunc(0x8a561d92, sys_heap_delete_heap);
	sysPrxForUser->AddFunc(0xb2fcf2c8, sys_heap_create_heap);
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

	sysPrxForUser->AddFunc(0x8c2bb498, sys_spinlock_initialize);
	sysPrxForUser->AddFunc(0xa285139d, sys_spinlock_lock);
	sysPrxForUser->AddFunc(0x722a0254, sys_spinlock_trylock);
	sysPrxForUser->AddFunc(0x5267cb35, sys_spinlock_unlock);

	sysPrxForUser->AddFunc(0x67f9fedb, sys_game_process_exitspawn2);
	sysPrxForUser->AddFunc(0xfc52a7a9, sys_game_process_exitspawn);

	sysPrxForUser->AddFunc(0x68b9b011, _sys_memset);
}
