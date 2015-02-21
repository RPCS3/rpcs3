#include "stdafx.h"
#include "Utilities/Log.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/SysCalls/Modules.h"
#include "Emu/SysCalls/CB_FUNC.h"
#include "Emu/Memory/atomic_type.h"

#include "Emu/FS/vfsFile.h"
#include "Emu/SysCalls/lv2/sleep_queue_type.h"
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

extern Module sysPrxForUser;

#define TLS_MAX 128
#define TLS_SYS 0x30

u32 g_tls_start; // start of TLS memory area
u32 g_tls_size;

std::array<std::atomic<u32>, TLS_MAX> g_tls_owners;

void sys_initialize_tls()
{
	sysPrxForUser.Log("sys_initialize_tls()");
}

u32 ppu_get_tls(u32 thread)
{
	if (!g_tls_start)
	{
		g_tls_size = Emu.GetTLSMemsz() + TLS_SYS;
		g_tls_start = Memory.MainMem.AllocAlign(g_tls_size * TLS_MAX, 4096); // memory for up to TLS_MAX threads
		sysPrxForUser.Notice("Thread Local Storage initialized (g_tls_start=0x%x, user_size=0x%x)\n*** TLS segment addr: 0x%08x\n*** TLS segment size: 0x%08x",
			g_tls_start, Emu.GetTLSMemsz(), Emu.GetTLSAddr(), Emu.GetTLSFilesz());
	}

	if (!thread)
	{
		return 0;
	}
	
	for (u32 i = 0; i < TLS_MAX; i++)
	{
		if (g_tls_owners[i] == thread)
		{
			return g_tls_start + i * g_tls_size + TLS_SYS; // if already initialized, return TLS address
		}
	}

	for (u32 i = 0; i < TLS_MAX; i++)
	{
		u32 old = 0;
		if (g_tls_owners[i].compare_exchange_strong(old, thread))
		{
			const u32 addr = g_tls_start + i * g_tls_size + TLS_SYS; // get TLS address
			memset(vm::get_ptr(addr - TLS_SYS), 0, TLS_SYS); // initialize system area with zeros
			memcpy(vm::get_ptr(addr), vm::get_ptr(Emu.GetTLSAddr()), Emu.GetTLSFilesz()); // initialize from TLS image
			memset(vm::get_ptr(addr + Emu.GetTLSFilesz()), 0, Emu.GetTLSMemsz() - Emu.GetTLSFilesz()); // fill the rest with zeros
			return addr;
		}
	}

	throw "Out of TLS memory";
}

void ppu_free_tls(u32 thread)
{
	for (auto& v : g_tls_owners)
	{
		u32 old = thread;
		if (v.compare_exchange_strong(old, 0))
		{
			return;
		}
	}
}

std::string ps3_fmt(PPUThread& context, vm::ptr<const char> fmt, u32 g_count, u32 f_count, u32 v_count)
{
	std::string result;

	for (char c = *fmt++; c; c = *fmt++)
	{
		switch (c)
		{
		case '%':
		{
			const auto start = fmt - 1;

			// read flags
			const bool plus_sign = *fmt == '+' ? fmt++, true : false;
			const bool minus_sign = *fmt == '-' ? fmt++, true : false;
			const bool space_sign = *fmt == ' ' ? fmt++, true : false;
			const bool number_sign = *fmt == '#' ? fmt++, true : false;
			const bool zero_padding = *fmt == '0' ? fmt++, true : false;

			// read width
			const u32 width = [&]() -> u32
			{
				u32 width = 0;

				if (*fmt == '*')
				{
					fmt++;
					return context.get_next_gpr_arg(g_count, f_count, v_count);
				}

				while (*fmt - '0' < 10)
				{
					width = width * 10 + (*fmt++ - '0');
				}

				return width;
			}();

			// read precision
			const u32 prec = [&]() -> u32
			{
				u32 prec = 0;

				if (*fmt != '.')
				{
					return 0;
				}

				if (*++fmt == '*')
				{
					fmt++;
					return context.get_next_gpr_arg(g_count, f_count, v_count);
				}

				while (*fmt - '0' < 10)
				{
					prec = prec * 10 + (*fmt++ - '0');
				}

				return prec;
			}();

			switch (char cf = *fmt++)
			{
			case '%':
			{
				if (plus_sign || minus_sign || space_sign || number_sign || zero_padding || width || prec) break;

				result += '%';
				continue;
			}
			case 'd':
			case 'i':
			{
				// signed decimal
				const s64 value = context.get_next_gpr_arg(g_count, f_count, v_count);

				if (plus_sign || minus_sign || space_sign || number_sign || zero_padding || width || prec) break;

				result += fmt::to_sdec(value);
				continue;
			}
			case 'x':
			case 'X':
			{
				// hexadecimal
				const u64 value = context.get_next_gpr_arg(g_count, f_count, v_count);

				if (plus_sign || minus_sign || space_sign || prec) break;

				if (number_sign && value)
				{
					result += cf == 'x' ? "0x" : "0X";
				}

				const std::string& hex = cf == 'x' ? fmt::to_hex(value) : fmt::toupper(fmt::to_hex(value));

				if (hex.length() >= width)
				{
					result += hex;
				}
				else if (zero_padding)
				{
					result += std::string(width - hex.length(), '0') + hex;
				}
				else
				{
					result += hex + std::string(width - hex.length(), ' ');
				}
				continue;
			}
			case 's':
			{
				// string
				auto string = vm::ptr<const char>::make(context.get_next_gpr_arg(g_count, f_count, v_count));

				if (plus_sign || minus_sign || space_sign || number_sign || zero_padding || width || prec) break;

				result += string.get_ptr();
				continue;
			}
			}

			throw fmt::format("ps3_fmt(): unknown formatting: '%s'", start.get_ptr());
		}
		}

		result += c;
	}

	return result;
}

int _sys_heap_create_heap(const u32 heap_addr, const u32 align, const u32 size)
{
	sysPrxForUser.Warning("_sys_heap_create_heap(heap_addr=0x%x, align=0x%x, size=0x%x)", heap_addr, align, size);

	std::shared_ptr<HeapInfo> heap(new HeapInfo(heap_addr, align, size));
	u32 heap_id = sysPrxForUser.GetNewId(heap);
	sysPrxForUser.Warning("*** sys_heap created: id = %d", heap_id);
	return heap_id;
}

u32 _sys_heap_malloc(const u32 heap_id, const u32 size)
{
	sysPrxForUser.Warning("_sys_heap_malloc(heap_id=%d, size=0x%x)", heap_id, size);

	std::shared_ptr<HeapInfo> heap;
	if(!sysPrxForUser.CheckId(heap_id, heap)) return CELL_ESRCH;

	return (u32)Memory.Alloc(size, 1);
}

u32 _sys_heap_memalign(u32 heap_id, u32 align, u32 size)
{
	sysPrxForUser.Warning("_sys_heap_memalign(heap_id=%d, align=0x%x, size=0x%x)", heap_id, align, size);

	std::shared_ptr<HeapInfo> heap;
	if(!sysPrxForUser.CheckId(heap_id, heap)) return CELL_ESRCH;

	return (u32)Memory.Alloc(size, align);
}

s64 _sys_process_atexitspawn()
{
	sysPrxForUser.Log("_sys_process_atexitspawn()");
	return CELL_OK;
}

s64 _sys_process_at_Exitspawn()
{
	sysPrxForUser.Log("_sys_process_at_Exitspawn");
	return CELL_OK;
}

int sys_process_is_stack(u32 p)
{
	sysPrxForUser.Log("sys_process_is_stack(p=0x%x)", p);

	// prx: compare high 4 bits with "0xD"
	return (p >= Memory.StackMem.GetStartAddr() && p <= Memory.StackMem.GetEndAddr()) ? 1 : 0;
}

s64 sys_prx_exitspawn_with_level()
{
	sysPrxForUser.Log("sys_prx_exitspawn_with_level()");
	return CELL_OK;
}

int sys_spu_elf_get_information(u32 elf_img, vm::ptr<u32> entry, vm::ptr<u32> nseg)
{
	sysPrxForUser.Todo("sys_spu_elf_get_information(elf_img=0x%x, entry_addr=0x%x, nseg_addr=0x%x)", elf_img, entry.addr(), nseg.addr());
	return CELL_OK;
}

int sys_spu_elf_get_segments(u32 elf_img, vm::ptr<sys_spu_segment> segments, int nseg)
{
	sysPrxForUser.Todo("sys_spu_elf_get_segments(elf_img=0x%x, segments_addr=0x%x, nseg=0x%x)", elf_img, segments.addr(), nseg);
	return CELL_OK;
}

int sys_spu_image_import(vm::ptr<sys_spu_image> img, u32 src, u32 type)
{
	sysPrxForUser.Warning("sys_spu_image_import(img=0x%x, src=0x%x, type=%d)", img.addr(), src, type);

	return spu_image_import(*img, src, type);
}

int sys_spu_image_close(vm::ptr<sys_spu_image> img)
{
	sysPrxForUser.Warning("sys_spu_image_close(img=0x%x)", img.addr());
	return CELL_OK;
}

int sys_raw_spu_load(s32 id, vm::ptr<const char> path, vm::ptr<u32> entry)
{
	sysPrxForUser.Warning("sys_raw_spu_load(id=0x%x, path_addr=0x%x('%s'), entry_addr=0x%x)", 
		id, path.addr(), path.get_ptr(), entry.addr());

	vfsFile f(path.get_ptr());
	if(!f.IsOpened())
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

	*entry = _entry;

	return CELL_OK;
}

int sys_raw_spu_image_load(int id, vm::ptr<sys_spu_image> img)
{
	sysPrxForUser.Warning("sys_raw_spu_image_load(id=0x%x, img_addr=0x%x)", id, img.addr());

	// TODO: use segment info
	memcpy(vm::get_ptr<void>(RAW_SPU_BASE_ADDR + RAW_SPU_OFFSET * id), vm::get_ptr<void>(img->addr), 256 * 1024);
	vm::write32(RAW_SPU_BASE_ADDR + RAW_SPU_OFFSET * id + RAW_SPU_PROB_OFFSET + SPU_NPC_offs, (u32)img->entry_point);

	return CELL_OK;
}

int sys_get_random_number(vm::ptr<u8> addr, u64 size)
{
	sysPrxForUser.Warning("sys_get_random_number(addr=0x%x, size=%d)", addr.addr(), size);

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
	sysPrxForUser.Log("_sys_memset(dst_addr=0x%x, value=%d, size=%d)", dst.addr(), value, size);

	memset(dst.get_ptr(), value, size);
	return dst;
}

vm::ptr<void> _sys_memcpy(vm::ptr<void> dst, vm::ptr<const void> src, u32 size)
{
	sysPrxForUser.Log("_sys_memcpy(dst_addr=0x%x, src_addr=0x%x, size=%d)", dst.addr(), src.addr(), size);

	memcpy(dst.get_ptr(), src.get_ptr(), size);
	return dst;
}

s32 _sys_memcmp(vm::ptr<const void> buf1, vm::ptr<const void> buf2, u32 size)
{
	sysPrxForUser.Log("_sys_memcmp(buf1_addr=0x%x, buf2_addr=0x%x, size=%d)", buf1.addr(), buf2.addr(), size);

	return memcmp(buf1.get_ptr(), buf2.get_ptr(), size);
}

s64 _sys_strlen(vm::ptr<const char> str)
{
	sysPrxForUser.Log("_sys_strlen(str_addr=0x%x)", str.addr());

	return strlen(str.get_ptr());
}

s32 _sys_strncmp(vm::ptr<const char> str1, vm::ptr<const char> str2, s32 max)
{
	sysPrxForUser.Log("_sys_strncmp(str1_addr=0x%x, str2_addr=0x%x, max=%d)", str1.addr(), str2.addr(), max);

	return strncmp(str1.get_ptr(), str2.get_ptr(), max);
}

vm::ptr<char> _sys_strcat(vm::ptr<char> dest, vm::ptr<const char> source)
{
	sysPrxForUser.Log("_sys_strcat(dest_addr=0x%x, source_addr=0x%x)", dest.addr(), source.addr());

	if (strcat(dest.get_ptr(), source.get_ptr()) != dest.get_ptr())
	{
		assert(!"strcat(): unexpected result");
	}
	return dest;
}

vm::ptr<char> _sys_strncat(vm::ptr<char> dest, vm::ptr<const char> source, u32 len)
{
	sysPrxForUser.Log("_sys_strncat(dest_addr=0x%x, source_addr=0x%x, len=%d)", dest.addr(), source.addr(), len);

	if (strncat(dest.get_ptr(), source.get_ptr(), len) != dest.get_ptr())
	{
		assert(!"strncat(): unexpected result");
	}
	return dest;
}

vm::ptr<char> _sys_strcpy(vm::ptr<char> dest, vm::ptr<const char> source)
{
	sysPrxForUser.Log("_sys_strcpy(dest_addr=0x%x, source_addr=0x%x)", dest.addr(), source.addr());

	if (strcpy(dest.get_ptr(), source.get_ptr()) != dest.get_ptr())
	{
		assert(!"strcpy(): unexpected result");
	}
	return dest;
}

vm::ptr<char> _sys_strncpy(vm::ptr<char> dest, vm::ptr<const char> source, u32 len)
{
	sysPrxForUser.Log("_sys_strncpy(dest_addr=0x%x, source_addr=0x%x, len=%d)", dest.addr(), source.addr(), len);

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
	sysPrxForUser.Warning("_sys_spu_printf_initialize(agcb_addr=0x%x, dgcb_addr=0x%x, atcb_addr=0x%x, dtcb_addr=0x%x)",
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
	sysPrxForUser.Warning("_sys_spu_printf_finalize()");

	spu_printf_agcb.set(0);
	spu_printf_dgcb.set(0);
	spu_printf_atcb.set(0);
	spu_printf_dtcb.set(0);
	return CELL_OK;
}

s32 _sys_spu_printf_attach_group(PPUThread& CPU, u32 group)
{
	sysPrxForUser.Warning("_sys_spu_printf_attach_group(group=%d)", group);

	if (!spu_printf_agcb)
	{
		return CELL_ESTAT;
	}

	return spu_printf_agcb(CPU, group);
}

s32 _sys_spu_printf_detach_group(PPUThread& CPU, u32 group)
{
	sysPrxForUser.Warning("_sys_spu_printf_detach_group(group=%d)", group);

	if (!spu_printf_dgcb)
	{
		return CELL_ESTAT;
	}

	return spu_printf_dgcb(CPU, group);
}

s32 _sys_spu_printf_attach_thread(PPUThread& CPU, u32 thread)
{
	sysPrxForUser.Warning("_sys_spu_printf_attach_thread(thread=%d)", thread);

	if (!spu_printf_atcb)
	{
		return CELL_ESTAT;
	}

	return spu_printf_atcb(CPU, thread);
}

s32 _sys_spu_printf_detach_thread(PPUThread& CPU, u32 thread)
{
	sysPrxForUser.Warning("_sys_spu_printf_detach_thread(thread=%d)", thread);

	if (!spu_printf_dtcb)
	{
		return CELL_ESTAT;
	}

	return spu_printf_dtcb(CPU, thread);
}

s32 _sys_snprintf(PPUThread& CPU, vm::ptr<char> dst, u32 count, vm::ptr<const char> fmt) // va_args...
{
	sysPrxForUser.Warning("_sys_snprintf(dst=0x%x, count=%d, fmt=0x%x, ...)", dst, count, fmt);

	std::string result = ps3_fmt(CPU, fmt, 3, 0, 0);

	sysPrxForUser.Warning("*** '%s' -> '%s'", fmt.get_ptr(), result);

	if (!count)
	{
		return 0; // ???
	}
	else
	{
		count = (u32)std::min<size_t>(count - 1, result.size());

		memcpy(dst.get_ptr(), result.c_str(), count);
		dst[count] = 0;
		return count;
	}
}

s32 _sys_printf(vm::ptr<const char> fmt) // va_args...
{
	sysPrxForUser.Todo("_sys_printf(fmt_addr=0x%x, ...)", fmt.addr());

	// probably, assertion failed
	sysPrxForUser.Warning("_sys_printf: \n%s", fmt.get_ptr());
	Emu.Pause();
	return CELL_OK;
}

s32 _nid_E75C40F2(u32 dest)
{
	sysPrxForUser.Todo("Unnamed function 0xE75C40F2 (dest=0x%x) -> CELL_ENOENT", dest);

	// prx: load some data (0x40 bytes) previously set by sys_process_get_paramsfo
	//memset(Memory + dest, 0, 0x40);
	return CELL_ENOENT;
}

Module sysPrxForUser("sysPrxForUser", []()
{
	g_tls_start = 0;
	for (auto& v : g_tls_owners)
	{
		v.store(0, std::memory_order_relaxed);
	}

	spu_printf_agcb.set(0);
	spu_printf_dgcb.set(0);
	spu_printf_atcb.set(0);
	spu_printf_dtcb.set(0);

	// Setup random number generator
	srand(time(NULL));

	REG_FUNC(sysPrxForUser, sys_initialize_tls);
	
	REG_FUNC(sysPrxForUser, sys_lwmutex_create);
	REG_FUNC(sysPrxForUser, sys_lwmutex_destroy);
	REG_FUNC(sysPrxForUser, sys_lwmutex_lock);
	REG_FUNC(sysPrxForUser, sys_lwmutex_trylock);
	REG_FUNC(sysPrxForUser, sys_lwmutex_unlock);

	REG_FUNC(sysPrxForUser, sys_time_get_system_time);

	REG_FUNC(sysPrxForUser, sys_process_exit);
	REG_FUNC(sysPrxForUser, _sys_process_atexitspawn);
	REG_FUNC(sysPrxForUser, _sys_process_at_Exitspawn);
	REG_FUNC(sysPrxForUser, sys_process_is_stack);

	REG_FUNC(sysPrxForUser, sys_ppu_thread_create);
	REG_FUNC(sysPrxForUser, sys_ppu_thread_get_id);
	REG_FUNC(sysPrxForUser, sys_ppu_thread_exit);
	REG_FUNC(sysPrxForUser, sys_ppu_thread_once);

	REG_FUNC(sysPrxForUser, sys_prx_load_module);
	REG_FUNC(sysPrxForUser, sys_prx_start_module);
	REG_FUNC(sysPrxForUser, sys_prx_stop_module);
	REG_FUNC(sysPrxForUser, sys_prx_unload_module);
	REG_FUNC(sysPrxForUser, sys_prx_register_library);
	REG_FUNC(sysPrxForUser, sys_prx_unregister_library);
	REG_FUNC(sysPrxForUser, sys_prx_get_module_list);
	REG_FUNC(sysPrxForUser, sys_prx_get_module_info);
	REG_FUNC(sysPrxForUser, sys_prx_get_module_id_by_name);
	REG_FUNC(sysPrxForUser, sys_prx_load_module_on_memcontainer);
	REG_FUNC(sysPrxForUser, sys_prx_exitspawn_with_level);

	REG_FUNC(sysPrxForUser, _sys_heap_malloc);
	//REG_FUNC(sysPrxForUser, _sys_heap_free);
	//REG_FUNC(sysPrxForUser, _sys_heap_delete_heap);
	REG_FUNC(sysPrxForUser, _sys_heap_create_heap);
	REG_FUNC(sysPrxForUser, _sys_heap_memalign);

	REG_FUNC(sysPrxForUser, sys_mmapper_allocate_memory);
	REG_FUNC(sysPrxForUser, sys_mmapper_allocate_memory_from_container);
	REG_FUNC(sysPrxForUser, sys_mmapper_map_memory);
	REG_FUNC(sysPrxForUser, sys_mmapper_unmap_memory);
	REG_FUNC(sysPrxForUser, sys_mmapper_free_memory);

	REG_FUNC(sysPrxForUser, sys_spu_elf_get_information);
	REG_FUNC(sysPrxForUser, sys_spu_elf_get_segments);
	REG_FUNC(sysPrxForUser, sys_spu_image_import);
	REG_FUNC(sysPrxForUser, sys_spu_image_close);

	REG_FUNC(sysPrxForUser, sys_raw_spu_load);
	REG_FUNC(sysPrxForUser, sys_raw_spu_image_load);

	REG_FUNC(sysPrxForUser, sys_lwcond_create);
	REG_FUNC(sysPrxForUser, sys_lwcond_destroy);
	REG_FUNC(sysPrxForUser, sys_lwcond_signal);
	REG_FUNC(sysPrxForUser, sys_lwcond_signal_all);
	REG_FUNC(sysPrxForUser, sys_lwcond_signal_to);
	REG_FUNC(sysPrxForUser, sys_lwcond_wait);

	REG_FUNC(sysPrxForUser, sys_get_random_number);

	REG_FUNC(sysPrxForUser, sys_spinlock_initialize);
	REG_FUNC(sysPrxForUser, sys_spinlock_lock);
	REG_FUNC(sysPrxForUser, sys_spinlock_trylock);
	REG_FUNC(sysPrxForUser, sys_spinlock_unlock);

	REG_FUNC(sysPrxForUser, sys_game_process_exitspawn2);
	REG_FUNC(sysPrxForUser, sys_game_process_exitspawn);

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

	REG_UNNAMED(sysPrxForUser, E75C40F2);
});
