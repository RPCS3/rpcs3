#include "stdafx.h"
#include "Utilities/Log.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/IdManager.h"
#include "Emu/SysCalls/Modules.h"
#include "Emu/SysCalls/CB_FUNC.h"

#include "Emu/FS/vfsFile.h"
#include "Emu/SysCalls/lv2/sleep_queue.h"
#include "Emu/SysCalls/lv2/sys_interrupt.h"
#include "Emu/SysCalls/lv2/sys_spu.h"
#include "Emu/SysCalls/lv2/sys_lwmutex.h"
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

waiter_map_t g_sys_spinlock_wm("sys_spinlock_wm");

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

s32 sys_lwmutex_create(vm::ptr<sys_lwmutex_t> lwmutex, vm::ptr<sys_lwmutex_attribute_t> attr)
{
	sysPrxForUser.Warning("sys_lwmutex_create(lwmutex=*0x%x, attr=*0x%x)", lwmutex, attr);

	const bool recursive = attr->recursive.data() == se32(SYS_SYNC_RECURSIVE);

	if (!recursive && attr->recursive.data() != se32(SYS_SYNC_NOT_RECURSIVE))
	{
		sysPrxForUser.Error("sys_lwmutex_create(): invalid recursive attribute (0x%x)", attr->recursive);
		return CELL_EINVAL;
	}

	const u32 protocol = attr->protocol;

	switch (protocol)
	{
	case SYS_SYNC_FIFO: break;
	case SYS_SYNC_RETRY: break;
	case SYS_SYNC_PRIORITY: break;
	default: sysPrxForUser.Error("sys_lwmutex_create(): invalid protocol (0x%x)", protocol); return CELL_EINVAL;
	}

	std::shared_ptr<lwmutex_t> lw(new lwmutex_t(protocol, attr->name_u64));

	lwmutex->lock_var = { { lwmutex::free, lwmutex::zero } };
	lwmutex->attribute = attr->recursive | attr->protocol;
	lwmutex->recursive_count = 0;
	lwmutex->sleep_queue = Emu.GetIdManager().GetNewID(lw);

	return CELL_OK;
}

s32 sys_lwmutex_destroy(PPUThread& CPU, vm::ptr<sys_lwmutex_t> lwmutex)
{
	sysPrxForUser.Log("sys_lwmutex_destroy(lwmutex=*0x%x)", lwmutex);

	// check to prevent recursive locking in the next call
	if (lwmutex->owner.read_relaxed() == CPU.GetId())
	{
		return CELL_EBUSY;
	}

	// attempt to lock the mutex
	if (s32 res = sys_lwmutex_trylock(CPU, lwmutex))
	{
		return res;
	}

	// call the syscall
	if (s32 res = _sys_lwmutex_destroy(lwmutex->sleep_queue))
	{
		// unlock the mutex if failed
		sys_lwmutex_unlock(CPU, lwmutex);

		return res;
	}

	// deleting succeeded
	lwmutex->owner.exchange(lwmutex::dead);

	return CELL_OK;
}

s32 sys_lwmutex_lock(PPUThread& CPU, vm::ptr<sys_lwmutex_t> lwmutex, u64 timeout)
{
	sysPrxForUser.Log("sys_lwmutex_lock(lwmutex=*0x%x, timeout=0x%llx)", lwmutex, timeout);

	const be_t<u32> tid = be_t<u32>::make(CPU.GetId());

	// try to lock lightweight mutex
	const be_t<u32> old_owner = lwmutex->owner.compare_and_swap(lwmutex::free, tid);

	if (old_owner.data() == se32(lwmutex_free))
	{
		// locking succeeded
		return CELL_OK;
	}

	if (old_owner.data() == tid.data())
	{
		// recursive locking

		if ((lwmutex->attribute.data() & se32(SYS_SYNC_RECURSIVE)) == 0)
		{
			// if not recursive
			return CELL_EDEADLK;
		}

		if (lwmutex->recursive_count.data() == -1)
		{
			// if recursion limit reached
			return CELL_EKRESOURCE;
		}

		// recursive locking succeeded
		lwmutex->recursive_count++;
		lwmutex->lock_var.read_sync();

		return CELL_OK;
	}

	if (old_owner.data() == se32(lwmutex_dead))
	{
		// invalid or deleted mutex
		return CELL_EINVAL;
	}

	for (u32 i = 0; i < 300; i++)
	{
		if (lwmutex->owner.read_relaxed().data() == se32(lwmutex_free))
		{
			if (lwmutex->owner.compare_and_swap_test(lwmutex::free, tid))
			{
				// locking succeeded
				return CELL_OK;
			}
		}
	}

	// atomically increment waiter value using 64 bit op
	lwmutex->all_info++;

	if (lwmutex->owner.compare_and_swap_test(lwmutex::free, tid))
	{
		// locking succeeded
		lwmutex->all_info--;

		return CELL_OK;
	}

	// lock using the syscall
	const s32 res = _sys_lwmutex_lock(lwmutex->sleep_queue, timeout);

	lwmutex->all_info--;

	if (res == CELL_OK)
	{
		// locking succeeded
		auto old = lwmutex->owner.exchange(tid);

<<<<<<< HEAD
		if (old.data() != se32(lwmutex_reserved) && !Emu.IsStopped())
=======
		if (old.data() != se32(lwmutex_reserved))
>>>>>>> 6894ec113f7a436851e93e91270ba2cef56d00ef
		{
			sysPrxForUser.Fatal("sys_lwmutex_lock(lwmutex=*0x%x): locking failed (owner=0x%x)", lwmutex, old);
		}

		return CELL_OK;
	}

	if (res == CELL_EBUSY && lwmutex->attribute.data() & se32(SYS_SYNC_RETRY))
	{
		// TODO (protocol is ignored in current implementation)
		throw __FUNCTION__;
	}

	return res;
}

s32 sys_lwmutex_trylock(PPUThread& CPU, vm::ptr<sys_lwmutex_t> lwmutex)
{
	sysPrxForUser.Log("sys_lwmutex_trylock(lwmutex=*0x%x)", lwmutex);

	const be_t<u32> tid = be_t<u32>::make(CPU.GetId());

	// try to lock lightweight mutex
	const be_t<u32> old_owner = lwmutex->owner.compare_and_swap(lwmutex::free, tid);

	if (old_owner.data() == se32(lwmutex_free))
	{
		// locking succeeded
		return CELL_OK;
	}

	if (old_owner.data() == tid.data())
	{
		// recursive locking

		if ((lwmutex->attribute.data() & se32(SYS_SYNC_RECURSIVE)) == 0)
		{
			// if not recursive
			return CELL_EDEADLK;
		}

		if (lwmutex->recursive_count.data() == -1)
		{
			// if recursion limit reached
			return CELL_EKRESOURCE;
		}

		// recursive locking succeeded
		lwmutex->recursive_count++;
		lwmutex->lock_var.read_sync();

		return CELL_OK;
	}

	if (old_owner.data() == se32(lwmutex_dead))
	{
		// invalid or deleted mutex
		return CELL_EINVAL;
	}

	if (old_owner.data() == se32(lwmutex_reserved))
	{
		// should be locked by the syscall
		const s32 res = _sys_lwmutex_trylock(lwmutex->sleep_queue);

		if (res == CELL_OK)
		{
			// locking succeeded
			auto old = lwmutex->owner.exchange(tid);

<<<<<<< HEAD
			if (old.data() != se32(lwmutex_reserved) && !Emu.IsStopped())
=======
			if (old.data() != se32(lwmutex_reserved))
>>>>>>> 6894ec113f7a436851e93e91270ba2cef56d00ef
			{
				sysPrxForUser.Fatal("sys_lwmutex_trylock(lwmutex=*0x%x): locking failed (owner=0x%x)", lwmutex, old);
			}
		}

		return res;
	}

	// locked by another thread
	return CELL_EBUSY;
}

s32 sys_lwmutex_unlock(PPUThread& CPU, vm::ptr<sys_lwmutex_t> lwmutex)
{
	sysPrxForUser.Log("sys_lwmutex_unlock(lwmutex=*0x%x)", lwmutex);

	const be_t<u32> tid = be_t<u32>::make(CPU.GetId());

	// check owner
	if (lwmutex->owner.read_relaxed() != tid)
	{
		return CELL_EPERM;
	}

	if (lwmutex->recursive_count.data())
	{
		// recursive unlocking succeeded
		lwmutex->recursive_count--;

		return CELL_OK;
	}

	// ensure that waiter is zero
	if (lwmutex->lock_var.compare_and_swap_test({ tid, lwmutex::zero }, { lwmutex::free, lwmutex::zero }))
	{
		// unlocking succeeded
		return CELL_OK;
	}

	if (lwmutex->attribute.data() & se32(SYS_SYNC_RETRY))
	{
		// TODO (protocol is ignored in current implementation)
	}

	// set special value
	lwmutex->owner.exchange(lwmutex::reserved);

	// call the syscall
	if (_sys_lwmutex_unlock(lwmutex->sleep_queue) == CELL_ESRCH)
	{
		return CELL_ESRCH;
	}

	return CELL_OK;
}

s32 sys_lwcond_create(vm::ptr<sys_lwcond_t> lwcond, vm::ptr<sys_lwmutex_t> lwmutex, vm::ptr<sys_lwcond_attribute_t> attr)
{
	sysPrxForUser.Warning("sys_lwcond_create(lwcond=*0x%x, lwmutex=*0x%x, attr=*0x%x)", lwcond, lwmutex, attr);

	std::shared_ptr<lwcond_t> cond(new lwcond_t(attr->name_u64));

	lwcond->lwcond_queue = Emu.GetIdManager().GetNewID(cond, TYPE_LWCOND);
	lwcond->lwmutex = lwmutex;

	return CELL_OK;
}

s32 sys_lwcond_destroy(vm::ptr<sys_lwcond_t> lwcond)
{
	sysPrxForUser.Log("sys_lwcond_destroy(lwcond=*0x%x)", lwcond);

	const s32 res = _sys_lwcond_destroy(lwcond->lwcond_queue);

	if (res == CELL_OK)
	{
		lwcond->lwcond_queue = lwmutex_dead;
	}

	return res;
}

s32 sys_lwcond_signal(PPUThread& CPU, vm::ptr<sys_lwcond_t> lwcond)
{
	sysPrxForUser.Log("sys_lwcond_signal(lwcond=*0x%x)", lwcond);

	const vm::ptr<sys_lwmutex_t> lwmutex = lwcond->lwmutex;

	if ((lwmutex->attribute.data() & se32(SYS_SYNC_ATTR_PROTOCOL_MASK)) == se32(SYS_SYNC_RETRY))
	{
		// TODO (protocol ignored)
		//return _sys_lwcond_signal(lwcond->lwcond_queue, 0, -1, 2);
	}

	if (lwmutex->owner.read_relaxed() == CPU.GetId())
	{
		// if owns the mutex
		lwmutex->all_info++;

		// call the syscall
		if (s32 res = _sys_lwcond_signal(lwcond->lwcond_queue, lwmutex->sleep_queue, -1, 1))
		{
			lwmutex->all_info--;

			return res == CELL_EPERM ? CELL_OK : res;
		}

		return CELL_OK;
	}
	
	if (s32 res = sys_lwmutex_trylock(CPU, lwmutex))
	{
		// if locking failed

		if (res != CELL_EBUSY)
		{
			return CELL_ESRCH;
		}

		// call the syscall
		return _sys_lwcond_signal(lwcond->lwcond_queue, 0, -1, 2);
	}

	// if locking succeeded
	lwmutex->all_info++;

	// call the syscall
	if (s32 res = _sys_lwcond_signal(lwcond->lwcond_queue, lwmutex->sleep_queue, -1, 3))
	{
		lwmutex->all_info--;

		// unlock the lightweight mutex
		sys_lwmutex_unlock(CPU, lwmutex);

		return res == CELL_ENOENT ? CELL_OK : res;
	}

	return CELL_OK;
}

s32 sys_lwcond_signal_all(PPUThread& CPU, vm::ptr<sys_lwcond_t> lwcond)
{
	sysPrxForUser.Log("sys_lwcond_signal_all(lwcond=*0x%x)", lwcond);

	const vm::ptr<sys_lwmutex_t> lwmutex = lwcond->lwmutex;

	if ((lwmutex->attribute.data() & se32(SYS_SYNC_ATTR_PROTOCOL_MASK)) == se32(SYS_SYNC_RETRY))
	{
		// TODO (protocol ignored)
		//return _sys_lwcond_signal_all(lwcond->lwcond_queue, lwmutex->sleep_queue, 2);
	}

	if (lwmutex->owner.read_relaxed() == CPU.GetId())
	{
		// if owns the mutex, call the syscall
		const s32 res = _sys_lwcond_signal_all(lwcond->lwcond_queue, lwmutex->sleep_queue, 1);

		if (res <= 0)
		{
			// return error or CELL_OK
			return res;
		}

		lwmutex->all_info += res;

		return CELL_OK;
	}

	if (s32 res = sys_lwmutex_trylock(CPU, lwmutex))
	{
		// if locking failed

		if (res != CELL_EBUSY)
		{
			return CELL_ESRCH;
		}

		// call the syscall
		return _sys_lwcond_signal_all(lwcond->lwcond_queue, lwmutex->sleep_queue, 2);
	}

	// if locking succeeded, call the syscall
	s32 res = _sys_lwcond_signal_all(lwcond->lwcond_queue, lwmutex->sleep_queue, 1);

	if (res > 0)
	{
		lwmutex->all_info += res;

		res = CELL_OK;
	}

	// unlock mutex
	sys_lwmutex_unlock(CPU, lwmutex);

	return res;
}

s32 sys_lwcond_signal_to(PPUThread& CPU, vm::ptr<sys_lwcond_t> lwcond, u32 ppu_thread_id)
{
	sysPrxForUser.Log("sys_lwcond_signal_to(lwcond=*0x%x, ppu_thread_id=%d)", lwcond, ppu_thread_id);

	const vm::ptr<sys_lwmutex_t> lwmutex = lwcond->lwmutex;

	if ((lwmutex->attribute.data() & se32(SYS_SYNC_ATTR_PROTOCOL_MASK)) == se32(SYS_SYNC_RETRY))
	{
		// TODO (protocol ignored)
		//return _sys_lwcond_signal(lwcond->lwcond_queue, 0, ppu_thread_id, 2);
	}

	if (lwmutex->owner.read_relaxed() == CPU.GetId())
	{
		// if owns the mutex
		lwmutex->all_info++;

		// call the syscall
		if (s32 res = _sys_lwcond_signal(lwcond->lwcond_queue, lwmutex->sleep_queue, ppu_thread_id, 1))
		{
			lwmutex->all_info--;

			return res;
		}

		return CELL_OK;
	}

	if (s32 res = sys_lwmutex_trylock(CPU, lwmutex))
	{
		// if locking failed

		if (res != CELL_EBUSY)
		{
			return CELL_ESRCH;
		}

		// call the syscall
		return _sys_lwcond_signal(lwcond->lwcond_queue, 0, ppu_thread_id, 2);
	}

	// if locking succeeded
	lwmutex->all_info++;

	// call the syscall
	if (s32 res = _sys_lwcond_signal(lwcond->lwcond_queue, lwmutex->sleep_queue, ppu_thread_id, 3))
	{
		lwmutex->all_info--;

		// unlock the lightweight mutex
		sys_lwmutex_unlock(CPU, lwmutex);

		return res;
	}

	return CELL_OK;
}

s32 sys_lwcond_wait(PPUThread& CPU, vm::ptr<sys_lwcond_t> lwcond, u64 timeout)
{
	sysPrxForUser.Log("sys_lwcond_wait(lwcond=*0x%x, timeout=0x%llx)", lwcond, timeout);

	const be_t<u32> tid = be_t<u32>::make(CPU.GetId());

	const vm::ptr<sys_lwmutex_t> lwmutex = lwcond->lwmutex;

	if (lwmutex->owner.read_relaxed() != tid)
	{
		// if not owner of the mutex
		return CELL_EPERM;
	}

	// save old recursive value
	const be_t<u32> recursive_value = lwmutex->recursive_count;

	// set special value
	lwmutex->owner = { lwmutex::reserved };
	lwmutex->recursive_count = 0;

	// call the syscall
	s32 res = _sys_lwcond_queue_wait(lwcond->lwcond_queue, lwmutex->sleep_queue, timeout);

	if (res == CELL_OK || res == CELL_ESRCH)
	{
		if (res == CELL_OK)
		{
			lwmutex->all_info--;
		}

		// restore owner and recursive value
		const auto old = lwmutex->owner.exchange(tid);
		lwmutex->recursive_count = recursive_value;

<<<<<<< HEAD
		if (old.data() != se32(lwmutex_reserved) && !Emu.IsStopped())
=======
		if (old.data() != se32(lwmutex_reserved))
>>>>>>> 6894ec113f7a436851e93e91270ba2cef56d00ef
		{
			sysPrxForUser.Fatal("sys_lwcond_wait(lwcond=*0x%x): locking failed (lwmutex->owner=0x%x)", lwcond, old);
		}

		return res;
	}

	if (res == CELL_EBUSY || res == CELL_ETIMEDOUT)
	{
		const s32 res2 = sys_lwmutex_lock(CPU, lwmutex, 0);

		if (res2 == CELL_OK)
		{
			// if successfully locked, restore recursive value
			lwmutex->recursive_count = recursive_value;

			return res == CELL_EBUSY ? CELL_OK : res;
		}

		return res2;
	}

	if (res == CELL_EDEADLK)
	{
		// restore owner and recursive value
		const auto old = lwmutex->owner.exchange(tid);
		lwmutex->recursive_count = recursive_value;

<<<<<<< HEAD
		if (old.data() != se32(lwmutex_reserved) && !Emu.IsStopped())
=======
		if (old.data() != se32(lwmutex_reserved))
>>>>>>> 6894ec113f7a436851e93e91270ba2cef56d00ef
		{
			sysPrxForUser.Fatal("sys_lwcond_wait(lwcond=*0x%x): locking failed after timeout (lwmutex->owner=0x%x)", lwcond, old);
		}

		return CELL_ETIMEDOUT;
	}

	sysPrxForUser.Fatal("sys_lwcond_wait(lwcond=*0x%x): unexpected syscall result (0x%x)", lwcond, res);
	return res;
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

u32 _sys_heap_create_heap(vm::ptr<const char> name, u32 arg2, u32 arg3, u32 arg4)
{
	sysPrxForUser.Warning("_sys_heap_create_heap(name=*0x%x, arg2=0x%x, arg3=0x%x, arg4=0x%x)", name, arg2, arg3, arg4);

	std::shared_ptr<HeapInfo> heap(new HeapInfo(name.get_ptr()));

	return Emu.GetIdManager().GetNewID(heap);
}

s32 _sys_heap_delete_heap(u32 heap)
{
	sysPrxForUser.Warning("_sys_heap_delete_heap(heap=0x%x)", heap);

	Emu.GetIdManager().RemoveID<HeapInfo>(heap);

	return CELL_OK;
}

u32 _sys_heap_malloc(u32 heap, u32 size)
{
	sysPrxForUser.Warning("_sys_heap_malloc(heap=0x%x, size=0x%x)", heap, size);

	return Memory.MainMem.AllocAlign(size, 1);
}

u32 _sys_heap_memalign(u32 heap, u32 align, u32 size)
{
	sysPrxForUser.Warning("_sys_heap_memalign(heap=0x%x, align=0x%x, size=0x%x)", heap, align, size);

	return Memory.MainMem.AllocAlign(size, align);
}

s32 _sys_heap_free(u32 heap, u32 addr)
{
	sysPrxForUser.Warning("_sys_heap_free(heap=0x%x, addr=0x%x)", heap, addr);

	Memory.MainMem.Free(addr);

	return CELL_OK;
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

s32 sys_interrupt_thread_disestablish(PPUThread& CPU, u32 ih)
{
	sysPrxForUser.Todo("sys_interrupt_thread_disestablish(ih=%d)", ih);

	return _sys_interrupt_thread_disestablish(ih, vm::stackvar<be_t<u64>>(CPU));
}

s32 sys_process_is_stack(u32 p)
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

s32 sys_spu_elf_get_information(u32 elf_img, vm::ptr<u32> entry, vm::ptr<u32> nseg)
{
	sysPrxForUser.Todo("sys_spu_elf_get_information(elf_img=0x%x, entry_addr=0x%x, nseg_addr=0x%x)", elf_img, entry.addr(), nseg.addr());
	return CELL_OK;
}

s32 sys_spu_elf_get_segments(u32 elf_img, vm::ptr<sys_spu_segment> segments, s32 nseg)
{
	sysPrxForUser.Todo("sys_spu_elf_get_segments(elf_img=0x%x, segments_addr=0x%x, nseg=0x%x)", elf_img, segments.addr(), nseg);
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

s32 sys_raw_spu_load(s32 id, vm::ptr<const char> path, vm::ptr<u32> entry)
{
	sysPrxForUser.Warning("sys_raw_spu_load(id=%d, path=*0x%x, entry=*0x%x)", id, path, entry);
	sysPrxForUser.Warning("*** path = '%s'", path.get_ptr());

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

	*entry = _entry | 1;

	return CELL_OK;
}

s32 sys_raw_spu_image_load(s32 id, vm::ptr<sys_spu_image> img)
{
	sysPrxForUser.Warning("sys_raw_spu_image_load(id=%d, img=*0x%x)", id, img);

	// TODO: use segment info
	memcpy(vm::get_ptr<void>(RAW_SPU_BASE_ADDR + RAW_SPU_OFFSET * id), vm::get_ptr<void>(img->addr), 256 * 1024);
	vm::write32(RAW_SPU_BASE_ADDR + RAW_SPU_OFFSET * id + RAW_SPU_PROB_OFFSET + SPU_NPC_offs, img->entry_point | be_t<u32>::make(1));

	return CELL_OK;
}

s32 sys_get_random_number(vm::ptr<u8> addr, u64 size)
{
	sysPrxForUser.Warning("sys_get_random_number(addr=*0x%x, size=%d)", addr, size);

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
	sysPrxForUser.Log("_sys_memset(dst=*0x%x, value=%d, size=0x%x)", dst, value, size);

	memset(dst.get_ptr(), value, size);

	return dst;
}

vm::ptr<void> _sys_memcpy(vm::ptr<void> dst, vm::ptr<const void> src, u32 size)
{
	sysPrxForUser.Log("_sys_memcpy(dst=*0x%x, src=*0x%x, size=0x%x)", dst, src, size);

	memcpy(dst.get_ptr(), src.get_ptr(), size);

	return dst;
}

s32 _sys_memcmp(vm::ptr<const void> buf1, vm::ptr<const void> buf2, u32 size)
{
	sysPrxForUser.Log("_sys_memcmp(buf1=*0x%x, buf2=*0x%x, size=%d)", buf1, buf2, size);

	return memcmp(buf1.get_ptr(), buf2.get_ptr(), size);
}

s64 _sys_strlen(vm::ptr<const char> str)
{
	sysPrxForUser.Log("_sys_strlen(str=*0x%x)", str);

	return strlen(str.get_ptr());
}

s32 _sys_strcmp(vm::ptr<const char> str1, vm::ptr<const char> str2)
{
	sysPrxForUser.Log("_sys_strcmp(str1=*0x%x, str2=*0x%x)", str1, str2);

	return strcmp(str1.get_ptr(), str2.get_ptr());
}

s32 _sys_strncmp(vm::ptr<const char> str1, vm::ptr<const char> str2, s32 max)
{
	sysPrxForUser.Log("_sys_strncmp(str1=*0x%x, str2=*0x%x, max=%d)", str1, str2, max);

	return strncmp(str1.get_ptr(), str2.get_ptr(), max);
}

vm::ptr<char> _sys_strcat(vm::ptr<char> dest, vm::ptr<const char> source)
{
	sysPrxForUser.Log("_sys_strcat(dest=*0x%x, source=*0x%x)", dest, source);

	if (strcat(dest.get_ptr(), source.get_ptr()) != dest.get_ptr())
	{
		throw "_sys_strcat() failed: unexpected strcat() result";
	}

	return dest;
}

vm::ptr<char> _sys_strncat(vm::ptr<char> dest, vm::ptr<const char> source, u32 len)
{
	sysPrxForUser.Log("_sys_strncat(dest=*0x%x, source=*0x%x, len=%d)", dest, source, len);

	if (strncat(dest.get_ptr(), source.get_ptr(), len) != dest.get_ptr())
	{
		throw "_sys_strncat() failed: unexpected strncat() result";
	}

	return dest;
}

vm::ptr<char> _sys_strcpy(vm::ptr<char> dest, vm::ptr<const char> source)
{
	sysPrxForUser.Log("_sys_strcpy(dest=*0x%x, source=*0x%x)", dest, source);

	if (strcpy(dest.get_ptr(), source.get_ptr()) != dest.get_ptr())
	{
		throw "_sys_strcpy() failed: unexpected strcpy() result";
	}

	return dest;
}

vm::ptr<char> _sys_strncpy(vm::ptr<char> dest, vm::ptr<const char> source, u32 len)
{
	sysPrxForUser.Log("_sys_strncpy(dest=*0x%x, source=*0x%x, len=%d)", dest, source, len);

	if (!dest || !source)
	{
		return vm::ptr<char>::make(0);
	}

	if (strncpy(dest.get_ptr(), source.get_ptr(), len) != dest.get_ptr())
	{
		throw "_sys_strncpy() failed: unexpected strncpy() result";
	}

	return dest;
}

spu_printf_cb_t spu_printf_agcb;
spu_printf_cb_t spu_printf_dgcb;
spu_printf_cb_t spu_printf_atcb;
spu_printf_cb_t spu_printf_dtcb;

s32 _sys_spu_printf_initialize(spu_printf_cb_t agcb, spu_printf_cb_t dgcb, spu_printf_cb_t atcb, spu_printf_cb_t dtcb)
{
	sysPrxForUser.Warning("_sys_spu_printf_initialize(agcb=*0x%x, dgcb=*0x%x, atcb=*0x%x, dtcb=*0x%x)", agcb, dgcb, atcb, dtcb);

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

u32 _sys_malloc(u32 size)
{
	sysPrxForUser.Warning("_sys_malloc(size=0x%x)", size);

	return Memory.MainMem.AllocAlign(size, 1);
}

u32 _sys_memalign(u32 align, u32 size)
{
	sysPrxForUser.Warning("_sys_memalign(align=0x%x, size=0x%x)", align, size);

	return Memory.MainMem.AllocAlign(size, align);
}

s32 _sys_free(u32 addr)
{
	sysPrxForUser.Warning("_sys_free(addr=0x%x)", addr);

	Memory.MainMem.Free(addr);

	return CELL_OK;
}

s32 _sys_snprintf(PPUThread& CPU, vm::ptr<char> dst, u32 count, vm::ptr<const char> fmt) // va_args...
{
	sysPrxForUser.Warning("_sys_snprintf(dst=*0x%x, count=%d, fmt=*0x%x, ...)", dst, count, fmt);

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
	sysPrxForUser.Todo("_sys_printf(fmt=*0x%x, ...)", fmt);

	// probably, assertion failed
	sysPrxForUser.Warning("_sys_printf: \n%s", fmt.get_ptr());
	Emu.Pause();
	return CELL_OK;
}

s32 sys_process_get_paramsfo(vm::ptr<char> buffer)
{
	sysPrxForUser.Warning("sys_process_get_paramsfo(buffer=*0x%x)", buffer);

	// prx: load some data (0x40 bytes) previously set by _sys_process_get_paramsfo syscall
	return _sys_process_get_paramsfo(buffer);
}

void sys_spinlock_initialize(vm::ptr<atomic_t<u32>> lock)
{
	sysPrxForUser.Log("sys_spinlock_initialize(lock=*0x%x)", lock);

	// prx: set 0 and sync
	lock->exchange(be_t<u32>::make(0));
}

void sys_spinlock_lock(vm::ptr<atomic_t<u32>> lock)
{
	sysPrxForUser.Log("sys_spinlock_lock(lock=*0x%x)", lock);

	// prx: exchange with 0xabadcafe, repeat until exchanged with 0
	while (lock->exchange(be_t<u32>::make(0xabadcafe)).data())
	{
		g_sys_spinlock_wm.wait_op(lock.addr(), [lock](){ return lock->read_relaxed().data() == 0; });

		if (Emu.IsStopped())
		{
			sysPrxForUser.Warning("sys_spinlock_lock(lock=*0x%x) aborted", lock);
			break;
		}
	}
}

s32 sys_spinlock_trylock(vm::ptr<atomic_t<u32>> lock)
{
	sysPrxForUser.Log("sys_spinlock_trylock(lock=*0x%x)", lock);

	// prx: exchange with 0xabadcafe, translate exchanged value
	if (lock->exchange(be_t<u32>::make(0xabadcafe)).data())
	{
		return CELL_EBUSY;
	}

	return CELL_OK;
}

void sys_spinlock_unlock(vm::ptr<atomic_t<u32>> lock)
{
	sysPrxForUser.Log("sys_spinlock_unlock(lock=*0x%x)", lock);

	// prx: sync and set 0
	lock->exchange(be_t<u32>::make(0));

	g_sys_spinlock_wm.notify(lock.addr());
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

	REG_FUNC(sysPrxForUser, sys_lwcond_create);
	REG_FUNC(sysPrxForUser, sys_lwcond_destroy);
	REG_FUNC(sysPrxForUser, sys_lwcond_signal);
	REG_FUNC(sysPrxForUser, sys_lwcond_signal_all);
	REG_FUNC(sysPrxForUser, sys_lwcond_signal_to);
	REG_FUNC(sysPrxForUser, sys_lwcond_wait);

	REG_FUNC(sysPrxForUser, sys_time_get_system_time);

	REG_FUNC(sysPrxForUser, sys_process_exit);
	REG_FUNC(sysPrxForUser, _sys_process_atexitspawn);
	REG_FUNC(sysPrxForUser, _sys_process_at_Exitspawn);
	REG_FUNC(sysPrxForUser, sys_process_is_stack);

	REG_FUNC(sysPrxForUser, sys_interrupt_thread_disestablish);

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

	REG_FUNC(sysPrxForUser, _sys_heap_create_heap);
	REG_FUNC(sysPrxForUser, _sys_heap_delete_heap);
	REG_FUNC(sysPrxForUser, _sys_heap_malloc);
	REG_FUNC(sysPrxForUser, _sys_heap_memalign);
	REG_FUNC(sysPrxForUser, _sys_heap_free);

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
	REG_FUNC(sysPrxForUser, _sys_strcmp);
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

	REG_FUNC(sysPrxForUser, _sys_malloc);
	REG_FUNC(sysPrxForUser, _sys_memalign);
	REG_FUNC(sysPrxForUser, _sys_free);

	REG_FUNC(sysPrxForUser, _sys_snprintf);

	REG_FUNC(sysPrxForUser, _sys_printf);

	REG_FUNC(sysPrxForUser, sys_process_get_paramsfo);
});
