#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/SysCalls/Modules.h"

#include "Emu/FS/vfsFile.h"
#include "Emu/FS/vfsStreamMemory.h"
#include "Emu/SysCalls/lv2/sys_spu.h"
#include "Emu/SysCalls/lv2/sys_lwmutex.h"
#include "Emu/SysCalls/lv2/sys_spinlock.h"
#include "Emu/SysCalls/lv2/sys_prx.h"
#include "Emu/SysCalls/lv2/sys_ppu_thread.h"
#include "Emu/SysCalls/lv2/sys_process.h"
#include "Emu/SysCalls/lv2/sys_time.h"
#include "Emu/SysCalls/lv2/sys_mmapper.h"
#include "Emu/SysCalls/lv2/sys_lwcond.h"
#include "Loader/ELF.h"
#include "Emu/Cell/RawSPUThread.h"
#include "sysPrxForUser.h"

//void sysPrxForUser_init();
//Module sysPrxForUser("sysPrxForUser", sysPrxForUser_init);
Module *sysPrxForUser = nullptr;

extern u32 LoadSpuImage(vfsStream& stream, u32& spu_ep);

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

void sys_initialize_tls()
{
	sysPrxForUser->Log("sys_initialize_tls()");
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
	return (int)(bool)(p >= Memory.StackMem.GetStartAddr() && p <= Memory.StackMem.GetEndAddr());
}

s64 sys_prx_exitspawn_with_level()
{
	sysPrxForUser->Log("sys_prx_exitspawn_with_level()");
	return CELL_OK;
}

int sys_spu_elf_get_information(u32 elf_img, vm::ptr<be_t<u32>> entry, vm::ptr<be_t<u32>> nseg)
{
	sysPrxForUser->Todo("sys_spu_elf_get_information(elf_img=0x%x, entry_addr=0x%x, nseg_addr=0x%x", elf_img, entry.addr(), nseg.addr());
	return CELL_OK;
}

int sys_spu_elf_get_segments(u32 elf_img, vm::ptr<sys_spu_segment> segments, int nseg)
{
	sysPrxForUser->Todo("sys_spu_elf_get_segments(elf_img=0x%x, segments_addr=0x%x, nseg=0x%x)", elf_img, segments.addr(), nseg);
	return CELL_OK;
}

int sys_spu_image_import(vm::ptr<sys_spu_image> img, u32 src, u32 type)
{
	sysPrxForUser->Warning("sys_spu_image_import(img=0x%x, src=0x%x, type=0x%x)", img.addr(), src, type);

	vfsStreamMemory f(src);
	u32 entry;
	u32 offset = LoadSpuImage(f, entry);

	img->type = type;
	img->entry_point = entry;
	img->segs_addr = offset;
	img->nsegs = 0;

	return CELL_OK;
}

int sys_spu_image_close(vm::ptr<sys_spu_image> img)
{
	sysPrxForUser->Warning("sys_spu_image_close(img=0x%x)", img.addr());
	return CELL_OK;
}

int sys_raw_spu_load(int id, u32 path_addr, vm::ptr<be_t<u32>> entry)
{
	const std::string path = Memory.ReadString(path_addr);
	sysPrxForUser->Warning("sys_raw_spu_load(id=0x%x, path=0x%x [%s], entry_addr=0x%x)", 
		id, path_addr, path.c_str(), entry.addr());

	vfsFile f(path);
	if(!f.IsOpened())
	{
		sysPrxForUser->Error("sys_raw_spu_load error: '%s' not found!", path.c_str());
		return CELL_ENOENT;
	}

	ELFLoader l(f);
	l.LoadInfo();
	l.LoadData(RAW_SPU_BASE_ADDR + RAW_SPU_OFFSET * id);

	*entry = l.GetEntry();

	return CELL_OK;
}

int sys_raw_spu_image_load(int id, vm::ptr<sys_spu_image> img)
{
	sysPrxForUser->Warning("sys_raw_spu_image_load(id=0x%x, img_addr=0x%x)", id, img.addr());

	memcpy(Memory + (RAW_SPU_BASE_ADDR + RAW_SPU_OFFSET * id), Memory + (u32)img->segs_addr, 256 * 1024);
	Memory.Write32(RAW_SPU_BASE_ADDR + RAW_SPU_OFFSET * id + RAW_SPU_PROB_OFFSET + SPU_NPC_offs, (u32)img->entry_point);

	return CELL_OK;
}

u32 _sys_memset(u32 addr, s32 value, u32 size)
{
	sysPrxForUser->Log("_sys_memset(addr=0x%x, value=%d, size=%d)", addr, value, size);

	memset(Memory + addr, value, size);
	return addr;
}

u32 _sys_memcpy(u32 dest, u32 source, u32 size)
{
	sysPrxForUser->Log("_sys_memcpy(dest=0x%x, source=0x%x, size=%d)", dest, source, size);

	memcpy(Memory + dest, Memory + source, size);
	return dest;
}

s32 _sys_memcmp(u32 addr1, u32 addr2, u32 size)
{
	sysPrxForUser->Log("_sys_memcmp(addr1=0x%x, addr2=0x%x, size=%d)", addr1, addr2, size);

	return memcmp(Memory + addr1, Memory + addr2, size);
}

s64 _sys_strlen(u32 addr)
{
	sysPrxForUser->Log("_sys_strlen(addr=0x%x)", addr);

	return strlen((char*)(Memory + addr));
}

s32 _sys_strncmp(u32 str1, u32 str2, s32 max)
{
	sysPrxForUser->Log("_sys_strncmp(str1=0x%x, str2=0x%x, max=%d)", str1, str2, max);

	return strncmp((char*)(Memory + str1), (char*)(Memory + str2), max);
}

u32 _sys_strcat(u32 dest, u32 source)
{
	sysPrxForUser->Log("_sys_strcat(dest=0x%x, source=0x%x)", dest, source);

	assert(Memory.RealToVirtualAddr(strcat((char*)(Memory + dest), (char*)(Memory + source))) == dest);
	return dest;
}

u32 _sys_strncat(u32 dest, u32 source, u32 len)
{
	sysPrxForUser->Log("_sys_strncat(dest=0x%x, source=0x%x, len=%d)", dest, source, len);

	assert(Memory.RealToVirtualAddr(strncat((char*)(Memory + dest), (char*)(Memory + source), len)) == dest);
	return dest;
}

u32 _sys_strcpy(u32 dest, u32 source)
{
	sysPrxForUser->Log("_sys_strcpy(dest=0x%x, source=0x%x)", dest, source);

	assert(Memory.RealToVirtualAddr(strcpy((char*)(Memory + dest), (char*)(Memory + source))) == dest);
	return dest;
}

u32 _sys_strncpy(u32 dest, u32 source, u32 len)
{
	sysPrxForUser->Log("_sys_strncpy(dest=0x%x, source=0x%x, len=%d)", dest, source, len);

	if (!dest || !source)
	{
		return 0;
	}

	assert(Memory.RealToVirtualAddr(strncpy((char*)(Memory + dest), (char*)(Memory + source), len)) == dest);
	return dest;
}

u32 spu_printf_agcb;
u32 spu_printf_dgcb;
u32 spu_printf_atcb;
u32 spu_printf_dtcb;

s32 _sys_spu_printf_initialize(u32 agcb, u32 dgcb, u32 atcb, u32 dtcb)
{
	sysPrxForUser->Warning("_sys_spu_printf_initialize(agcb=0x%x, dgcb=0x%x, atcb=0x%x, dtcb=0x%x)", agcb, dgcb, atcb, dtcb);

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

	spu_printf_agcb = 0;
	spu_printf_dgcb = 0;
	spu_printf_atcb = 0;
	spu_printf_dtcb = 0;
	return CELL_OK;
}

s64 _sys_spu_printf_attach_group(u32 arg)
{
	sysPrxForUser->Warning("_sys_spu_printf_attach_group(arg=0x%x)", arg);

	if (!spu_printf_agcb)
	{
		return CELL_ESTAT;
	}

	return GetCurrentPPUThread().FastCall(Memory.Read32(spu_printf_agcb), Memory.Read32(spu_printf_agcb + 4), arg);
}

s64 _sys_spu_printf_detach_group(u32 arg)
{
	sysPrxForUser->Warning("_sys_spu_printf_detach_group(arg=0x%x)", arg);

	if (!spu_printf_dgcb)
	{
		return CELL_ESTAT;
	}

	return GetCurrentPPUThread().FastCall(Memory.Read32(spu_printf_dgcb), Memory.Read32(spu_printf_dgcb + 4), arg);
}

s64 _sys_spu_printf_attach_thread(u32 arg)
{
	sysPrxForUser->Warning("_sys_spu_printf_attach_thread(arg=0x%x)", arg);

	if (!spu_printf_atcb)
	{
		return CELL_ESTAT;
	}

	return GetCurrentPPUThread().FastCall(Memory.Read32(spu_printf_atcb), Memory.Read32(spu_printf_atcb + 4), arg);
}

s64 _sys_spu_printf_detach_thread(u32 arg)
{
	sysPrxForUser->Warning("_sys_spu_printf_detach_thread(arg=0x%x)", arg);

	if (!spu_printf_dtcb)
	{
		return CELL_ESTAT;
	}

	return GetCurrentPPUThread().FastCall(Memory.Read32(spu_printf_dtcb), Memory.Read32(spu_printf_dtcb + 4), arg);
}

s32 _sys_printf(u32 arg1)
{
	sysPrxForUser->Todo("_sys_printf(arg1=0x%x)", arg1);

	// probably, assertion failed
	sysPrxForUser->Warning("_sys_printf: \n%s", (char*)(Memory + arg1));
	return CELL_OK;
}

s32 _unnamed_E75C40F2(u32 dest)
{
	sysPrxForUser->Todo("Unnamed function 0xE75C40F2 (dest=0x%x) -> CELL_ENOENT", dest);

	// prx: load some data (0x40 bytes) previously set by sys_process_get_paramsfo
	//memset(Memory + dest, 0, 0x40);
	return CELL_ENOENT;
}

void sysPrxForUser_init()
{
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
	sysPrxForUser->AddFunc(0xe75c40f2, _unnamed_E75C40F2); // real name is unknown

	REG_FUNC(sysPrxForUser, _sys_spu_printf_initialize);
	REG_FUNC(sysPrxForUser, _sys_spu_printf_finalize);
	REG_FUNC(sysPrxForUser, _sys_spu_printf_attach_group);
	REG_FUNC(sysPrxForUser, _sys_spu_printf_detach_group);
	REG_FUNC(sysPrxForUser, _sys_spu_printf_attach_thread);
	REG_FUNC(sysPrxForUser, _sys_spu_printf_detach_thread);

	REG_FUNC(sysPrxForUser, _sys_printf);
}

void sysPrxForUser_load()
{
	spu_printf_agcb = 0;
	spu_printf_dgcb = 0;
	spu_printf_atcb = 0;
	spu_printf_dtcb = 0;
}