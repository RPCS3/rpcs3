#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/SysCalls/Modules.h"
#include "Emu/SysCalls/CB_FUNC.h"
#include "Emu/Memory/atomic_type.h"

#include "Emu/Cell/SPUThread.h"
#include "Emu/SysCalls/lv2/sleep_queue_type.h"
#include "Emu/SysCalls/lv2/sys_lwmutex.h"
#include "Emu/SysCalls/lv2/sys_lwcond.h"
#include "Emu/SysCalls/lv2/sys_spu.h"
#include "Emu/SysCalls/lv2/sys_ppu_thread.h"
#include "Emu/SysCalls/lv2/sys_memory.h"
#include "Emu/SysCalls/lv2/sys_process.h"
#include "Emu/SysCalls/lv2/sys_semaphore.h"
#include "Emu/SysCalls/lv2/sys_event.h"
#include "Emu/Cell/SPURSManager.h"
#include "sysPrxForUser.h"
#include "cellSpurs.h"

Module *cellSpurs = nullptr;

#ifdef PRX_DEBUG
extern u32 libsre;
extern u32 libsre_rtoc;
#endif

void spursKernelMain(SPUThread & spu);
s64 cellSpursLookUpTasksetAddress(vm::ptr<CellSpurs> spurs, vm::ptr<CellSpursTaskset> taskset, u32 id);
s64 _cellSpursSendSignal(vm::ptr<CellSpursTaskset> taskset, u32 taskID);

s64 spursCreateLv2EventQueue(vm::ptr<CellSpurs> spurs, u32& queue_id, vm::ptr<u8> port, s32 size, u64 name_u64)
{
#ifdef PRX_DEBUG_XXX
	vm::var<be_t<u32>> queue;
	s32 res = cb_call<s32, vm::ptr<CellSpurs>, vm::ptr<u32>, vm::ptr<u8>, s32, u32>(GetCurrentPPUThread(), libsre + 0xB14C, libsre_rtoc,
		spurs, queue, port, size, vm::read32(libsre_rtoc - 0x7E2C));
	queue_id = queue;
	return res;
#endif

	queue_id = event_queue_create(SYS_SYNC_FIFO, SYS_PPU_QUEUE, name_u64, 0, size);
	if (!queue_id)
	{
		return CELL_EAGAIN; // rough
	}

	if (s32 res = (s32)spursAttachLv2EventQueue(spurs, queue_id, port, 1, true))
	{
		assert(!"spursAttachLv2EventQueue() failed");
	}
	return CELL_OK;
}

s64 spursInit(
	vm::ptr<CellSpurs> spurs,
	const u32 revision,
	const u32 sdkVersion,
	const s32 nSpus,
	const s32 spuPriority,
	const s32 ppuPriority,
	u32 flags, // SpursAttrFlags
	const char prefix[],
	const u32 prefixSize,
	const u32 container,
	const u8 swlPriority[],
	const u32 swlMaxSpu,
	const u32 swlIsPreem)
{
#ifdef PRX_DEBUG_XXX
	return cb_call<s32, vm::ptr<CellSpurs>, u32, u32, s32, s32, s32, u32, u32, u32, u32, u32, u32, u32>(GetCurrentPPUThread(), libsre + 0x74E4, libsre_rtoc,
		spurs, revision, sdkVersion, nSpus, spuPriority, ppuPriority, flags, vm::get_addr(prefix), prefixSize, container, vm::get_addr(swlPriority), swlMaxSpu, swlIsPreem);
#endif

	// SPURS initialization (asserts should actually rollback and return the error instead)

	if (!spurs)
	{
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;
	}
	if (spurs.addr() % 128)
	{
		return CELL_SPURS_CORE_ERROR_ALIGN;
	}
	if (prefixSize > CELL_SPURS_NAME_MAX_LENGTH)
	{
		return CELL_SPURS_CORE_ERROR_INVAL;
	}
	if (process_is_spu_lock_line_reservation_address(spurs.addr(), SYS_MEMORY_ACCESS_RIGHT_SPU_THR) != CELL_OK)
	{
		return CELL_SPURS_CORE_ERROR_PERM;
	}

	const bool isSecond = (flags & SAF_SECOND_VERSION) != 0;
	memset(spurs.get_ptr(), 0, CellSpurs::size1 + isSecond * CellSpurs::size2);
	spurs->m.revision = revision;
	spurs->m.sdkVersion = sdkVersion;
	spurs->m.ppu0 = 0xffffffffull;
	spurs->m.ppu1 = 0xffffffffull;
	spurs->m.flags = flags;
	memcpy(spurs->m.prefix, prefix, prefixSize);
	spurs->m.prefixSize = (u8)prefixSize;

	std::string name(prefix, prefixSize); // initialize name string

	if (!isSecond)
	{
		spurs->m.wklMskA.write_relaxed(be_t<u32>::make(0xffff));
	}
	spurs->m.xCC = 0;
	spurs->m.xCD = 0;
	spurs->m.sysSrvMsgUpdateTrace = 0;
	for (u32 i = 0; i < 8; i++)
	{
		spurs->m.sysSrvWorkload[i] = -1;
	}

	// default or system workload:
#ifdef PRX_DEBUG
	spurs->m.wklInfoSysSrv.addr.set(be_t<u64>::make(vm::read32(libsre_rtoc - 0x7EA4)));
	spurs->m.wklInfoSysSrv.size = 0x2200;
#else
	spurs->m.wklInfoSysSrv.addr.set(be_t<u64>::make(SPURS_IMG_ADDR_SYS_SRV_WORKLOAD));
#endif
	spurs->m.wklInfoSysSrv.arg = 0;
	spurs->m.wklInfoSysSrv.uniqueId.write_relaxed(0xff);
	u32 sem;
	for (u32 i = 0; i < 0x10; i++)
	{
		sem = semaphore_create(0, 1, SYS_SYNC_PRIORITY, *(u64*)"_spuWkl");
		assert(sem && ~sem); // should rollback if semaphore creation failed and return the error
		spurs->m.wklF1[i].sem = sem;
	}
	if (isSecond)
	{
		for (u32 i = 0; i < 0x10; i++)
		{
			sem = semaphore_create(0, 1, SYS_SYNC_PRIORITY, *(u64*)"_spuWkl");
			assert(sem && ~sem);
			spurs->m.wklF2[i].sem = sem;
		}
	}
	sem = semaphore_create(0, 1, SYS_SYNC_PRIORITY, *(u64*)"_spuPrv");
	assert(sem && ~sem);
	spurs->m.semPrv = sem;
	spurs->m.unk11 = -1;
	spurs->m.unk12 = -1;
	spurs->m.unk13 = 0;
	spurs->m.nSpus = nSpus;
	spurs->m.spuPriority = spuPriority;
#ifdef PRX_DEBUG
	if (s32 res = spu_image_import(spurs->m.spuImg, vm::read32(libsre_rtoc - (isSecond ? 0x7E94 : 0x7E98)), 1))
	{
		assert(!"spu_image_import() failed");
	}
#else
	spurs->m.spuImg.addr = (u32)Memory.Alloc(0x40000, 4096);
#endif

	s32 tgt = SYS_SPU_THREAD_GROUP_TYPE_NORMAL;
	if (flags & SAF_SPU_TGT_EXCLUSIVE_NON_CONTEXT)
	{
		tgt = SYS_SPU_THREAD_GROUP_TYPE_EXCLUSIVE_NON_CONTEXT;
	}
	else if (flags & SAF_UNKNOWN_FLAG_0)
	{
		tgt = 0xC02;
	}
	if (flags & SAF_SPU_MEMORY_CONTAINER_SET) tgt |= SYS_SPU_THREAD_GROUP_TYPE_MEMORY_FROM_CONTAINER;
	if (flags & SAF_SYSTEM_WORKLOAD_ENABLED) tgt |= SYS_SPU_THREAD_GROUP_TYPE_COOPERATE_WITH_SYSTEM;
	if (flags & SAF_UNKNOWN_FLAG_7) tgt |= 0x102;
	if (flags & SAF_UNKNOWN_FLAG_8) tgt |= 0xC02;
	if (flags & SAF_UNKNOWN_FLAG_9) tgt |= 0x800;
	auto tg = spu_thread_group_create(name + "CellSpursKernelGroup", nSpus, spuPriority, tgt, container);
	assert(tg);
	spurs->m.spuTG = tg->m_id;

	name += "CellSpursKernel0";
	for (s32 num = 0; num < nSpus; num++, name[name.size() - 1]++)
	{
		spurs->m.spus[num] = spu_thread_initialize(tg, num, spurs->m.spuImg, name, SYS_SPU_THREAD_OPTION_DEC_SYNC_TB_ENABLE, 0, 0, 0, 0, [spurs, num](SPUThread& SPU)
		{
			SPU.GPR[3]._u32[3] = num;
			SPU.GPR[4]._u64[1] = spurs.addr();

#ifdef PRX_DEBUG_XXX
			return SPU.FastCall(SPU.PC);
#endif

			spursKernelMain(SPU);
		})->GetId();
	}

	if (flags & SAF_SPU_PRINTF_ENABLED)
	{
		// spu_printf: attach group
		if (!spu_printf_agcb || spu_printf_agcb(tg->m_id) != CELL_OK)
		{
			// remove flag if failed
			spurs->m.flags &= ~SAF_SPU_PRINTF_ENABLED;
		}
	}

	if (s32 res = lwmutex_create(spurs->m.mutex, SYS_SYNC_PRIORITY, SYS_SYNC_NOT_RECURSIVE, *(u64*)"_spuPrv"))
	{
		assert(!"lwmutex_create() failed");
	}
	if (s32 res = lwcond_create(spurs->m.cond, spurs->m.mutex, *(u64*)"_spuPrv"))
	{
		assert(!"lwcond_create() failed");
	}

	spurs->m.flags1 = (flags & SAF_EXIT_IF_NO_WORK ? SF1_EXIT_IF_NO_WORK : 0) | (isSecond ? SF1_32_WORKLOADS : 0);
	spurs->m.wklFlagReceiver.write_relaxed(0xff);
	spurs->m.wklFlag.flag.write_relaxed(be_t<u32>::make(-1));
	spurs->_u8[0xD64] = 0;
	spurs->_u8[0xD65] = 0;
	spurs->_u8[0xD66] = 0;
	spurs->m.ppuPriority = ppuPriority;

	u32 queue;
	if (s32 res = (s32)spursCreateLv2EventQueue(spurs, queue, vm::ptr<u8>::make(spurs.addr() + 0xc9), 0x2a, *(u64*)"_spuPrv"))
	{
		assert(!"spursCreateLv2EventQueue() failed");
	}
	spurs->m.queue = queue;

	u32 port = event_port_create(0);
	assert(port && ~port);
	spurs->m.port = port;

	if (s32 res = sys_event_port_connect_local(port, queue))
	{
		assert(!"sys_event_port_connect_local() failed");
	}

	name = std::string(prefix, prefixSize);

	spurs->m.ppu0 = ppu_thread_create(0, 0, ppuPriority, 0x4000, true, false, name + "SpursHdlr0", [spurs](PPUThread& CPU)
	{
#ifdef PRX_DEBUG_XXX
		return cb_call<void, vm::ptr<CellSpurs>>(CPU, libsre + 0x9214, libsre_rtoc, spurs);
#endif

		if (spurs->m.flags & SAF_UNKNOWN_FLAG_30)
		{
			return;
		}

		while (true)
		{
			if (Emu.IsStopped())
			{
				cellSpurs->Warning("SPURS Handler Thread 0 aborted");
				return;
			}

			if (spurs->m.flags1 & SF1_EXIT_IF_NO_WORK)
			{
				if (s32 res = sys_lwmutex_lock(CPU, spurs->get_lwmutex(), 0))
				{
					assert(!"sys_lwmutex_lock() failed");
				}
				if (spurs->m.xD66.read_relaxed())
				{
					if (s32 res = sys_lwmutex_unlock(CPU, spurs->get_lwmutex()))
					{
						assert(!"sys_lwmutex_unlock() failed");
					}
					return;
				}
				else while (true)
				{
					if (Emu.IsStopped()) break;

					spurs->m.xD64.exchange(0);
					if (spurs->m.exception.data() == 0)
					{
						bool do_break = false;
						for (u32 i = 0; i < 16; i++)
						{
							if (spurs->m.wklState1[i].read_relaxed() == 2 &&
								*((u64 *)spurs->m.wklInfo1[i].priority) != 0 &&
								spurs->m.wklMaxContention[i].read_relaxed() & 0xf
								)
							{
								if (spurs->m.wklReadyCount1[i].read_relaxed() ||
									spurs->m.wklSignal1.read_relaxed() & (0x8000u >> i) ||
									(spurs->m.wklFlag.flag.read_relaxed() == 0 &&
									spurs->m.wklFlagReceiver.read_relaxed() == (u8)i
									))
								{
									do_break = true;
									break;
								}
							}
						}
						if (spurs->m.flags1 & SF1_32_WORKLOADS) for (u32 i = 0; i < 16; i++)
						{
							if (spurs->m.wklState2[i].read_relaxed() == 2 &&
								*((u64 *)spurs->m.wklInfo2[i].priority) != 0 &&
								spurs->m.wklMaxContention[i].read_relaxed() & 0xf0
								)
							{
								if (spurs->m.wklIdleSpuCountOrReadyCount2[i].read_relaxed() ||
									spurs->m.wklSignal2.read_relaxed() & (0x8000u >> i) ||
									(spurs->m.wklFlag.flag.read_relaxed() == 0 &&
									spurs->m.wklFlagReceiver.read_relaxed() == (u8)i + 0x10
									))
								{
									do_break = true;
									break;
								}
							}
						}
						if (do_break) break; // from while
					}

					spurs->m.xD65.exchange(1);
					if (spurs->m.xD64.read_relaxed() == 0)
					{
						if (s32 res = sys_lwcond_wait(CPU, spurs->get_lwcond(), 0))
						{
							assert(!"sys_lwcond_wait() failed");
						}
					}
					spurs->m.xD65.exchange(0);
					if (spurs->m.xD66.read_relaxed())
					{
						if (s32 res = sys_lwmutex_unlock(CPU, spurs->get_lwmutex()))
						{
							assert(!"sys_lwmutex_unlock() failed");
						}
						return;
					}
				}

				if (Emu.IsStopped()) continue;

				if (s32 res = sys_lwmutex_unlock(CPU, spurs->get_lwmutex()))
				{
					assert(!"sys_lwmutex_unlock() failed");
				}
			}

			if (Emu.IsStopped()) continue;

			if (s32 res = sys_spu_thread_group_start(spurs->m.spuTG))
			{
				assert(!"sys_spu_thread_group_start() failed");
			}
			if (s32 res = sys_spu_thread_group_join(spurs->m.spuTG, vm::ptr<u32>::make(0), vm::ptr<u32>::make(0)))
			{
				if (res == CELL_ESTAT)
				{
					return;
				}
				assert(!"sys_spu_thread_group_join() failed");
			}

			if (Emu.IsStopped()) continue;

			if ((spurs->m.flags1 & SF1_EXIT_IF_NO_WORK) == 0)
			{
				assert(spurs->m.xD66.read_relaxed() == 1 || Emu.IsStopped());
				return;
			}
		}
	})->GetId();

	spurs->m.ppu1 = ppu_thread_create(0, 0, ppuPriority, 0x8000, true, false, name + "SpursHdlr1", [spurs](PPUThread& CPU)
	{
#ifdef PRX_DEBUG
		return cb_call<void, vm::ptr<CellSpurs>>(CPU, libsre + 0xB40C, libsre_rtoc, spurs);
#endif

	})->GetId();

	// enable exception event handler
	if (spurs->m.enableEH.compare_and_swap_test(be_t<u32>::make(0), be_t<u32>::make(1)))
	{
		if (s32 res = sys_spu_thread_group_connect_event(spurs->m.spuTG, spurs->m.queue, SYS_SPU_THREAD_GROUP_EVENT_EXCEPTION))
		{
			assert(!"sys_spu_thread_group_connect_event() failed");
		}
	}
	
	spurs->m.traceBuffer.set(0);
	// can also use cellLibprof if available (omitted)

	// some unknown subroutine
	spurs->m.sub3.unk1 = spurs.addr() + 0xc9;
	spurs->m.sub3.unk2 = 3; // unknown const
	spurs->m.sub3.port = (u64)spurs->m.port;

	if (flags & SAF_SYSTEM_WORKLOAD_ENABLED) // initialize system workload
	{
		s32 res = CELL_OK;
#ifdef PRX_DEBUG
		res = cb_call<s32, vm::ptr<CellSpurs>, u32, u32, u32>(GetCurrentPPUThread(), libsre + 0x10428, libsre_rtoc,
			spurs, vm::get_addr(swlPriority), swlMaxSpu, swlIsPreem);
#endif
		assert(res == CELL_OK);
	}
	else if (flags & SAF_EXIT_IF_NO_WORK) // wakeup
	{
		return spursWakeUp(GetCurrentPPUThread(), spurs);
	}

	return CELL_OK;
}

s64 cellSpursInitialize(vm::ptr<CellSpurs> spurs, s32 nSpus, s32 spuPriority, s32 ppuPriority, bool exitIfNoWork)
{
	cellSpurs->Warning("cellSpursInitialize(spurs_addr=0x%x, nSpus=%d, spuPriority=%d, ppuPriority=%d, exitIfNoWork=%d)",
		spurs.addr(), nSpus, spuPriority, ppuPriority, exitIfNoWork ? 1 : 0);
#ifdef PRX_DEBUG_XXX
	return GetCurrentPPUThread().FastCall2(libsre + 0x8480, libsre_rtoc);
#endif

	return spursInit(
		spurs,
		0,
		0,
		nSpus,
		spuPriority,
		ppuPriority,
		exitIfNoWork ? SAF_EXIT_IF_NO_WORK : SAF_NONE,
		nullptr,
		0,
		0,
		nullptr,
		0,
		0);
}

s64 cellSpursInitializeWithAttribute(vm::ptr<CellSpurs> spurs, vm::ptr<const CellSpursAttribute> attr)
{
	cellSpurs->Warning("cellSpursInitializeWithAttribute(spurs_addr=0x%x, attr_addr=0x%x)", spurs.addr(), attr.addr());
#ifdef PRX_DEBUG_XXX
	return GetCurrentPPUThread().FastCall2(libsre + 0x839C, libsre_rtoc);
#endif

	if (!attr)
	{
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;
	}
	if (attr.addr() % 8)
	{
		return CELL_SPURS_CORE_ERROR_ALIGN;
	}
	if (attr->m.revision > 2)
	{
		return CELL_SPURS_CORE_ERROR_INVAL;
	}
	
	return spursInit(
		spurs,
		attr->m.revision,
		attr->m.sdkVersion,
		attr->m.nSpus,
		attr->m.spuPriority,
		attr->m.ppuPriority,
		attr->m.flags | (attr->m.exitIfNoWork ? SAF_EXIT_IF_NO_WORK : 0),
		attr->m.prefix,
		attr->m.prefixSize,
		attr->m.container,
		attr->m.swlPriority,
		attr->m.swlMaxSpu,
		attr->m.swlIsPreem);
}

s64 cellSpursInitializeWithAttribute2(vm::ptr<CellSpurs> spurs, vm::ptr<const CellSpursAttribute> attr)
{
	cellSpurs->Warning("cellSpursInitializeWithAttribute2(spurs_addr=0x%x, attr_addr=0x%x)", spurs.addr(), attr.addr());
#ifdef PRX_DEBUG_XXX
	return GetCurrentPPUThread().FastCall2(libsre + 0x82B4, libsre_rtoc);
#endif

	if (!attr)
	{
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;
	}
	if (attr.addr() % 8)
	{
		return CELL_SPURS_CORE_ERROR_ALIGN;
	}
	if (attr->m.revision > 2)
	{
		return CELL_SPURS_CORE_ERROR_INVAL;
	}

	return spursInit(
		spurs,
		attr->m.revision,
		attr->m.sdkVersion,
		attr->m.nSpus,
		attr->m.spuPriority,
		attr->m.ppuPriority,
		attr->m.flags | (attr->m.exitIfNoWork ? SAF_EXIT_IF_NO_WORK : 0) | SAF_SECOND_VERSION,
		attr->m.prefix,
		attr->m.prefixSize,
		attr->m.container,
		attr->m.swlPriority,
		attr->m.swlMaxSpu,
		attr->m.swlIsPreem);
}

s64 _cellSpursAttributeInitialize(vm::ptr<CellSpursAttribute> attr, u32 revision, u32 sdkVersion, u32 nSpus, s32 spuPriority, s32 ppuPriority, bool exitIfNoWork)
{
	cellSpurs->Warning("_cellSpursAttributeInitialize(attr_addr=0x%x, revision=%d, sdkVersion=0x%x, nSpus=%d, spuPriority=%d, ppuPriority=%d, exitIfNoWork=%d)",
		attr.addr(), revision, sdkVersion, nSpus, spuPriority, ppuPriority, exitIfNoWork ? 1 : 0);
#ifdef PRX_DEBUG_XXX
	return GetCurrentPPUThread().FastCall2(libsre + 0x72CC, libsre_rtoc);
#endif

	if (!attr)
	{
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;
	}
	if (attr.addr() % 8)
	{
		return CELL_SPURS_CORE_ERROR_ALIGN;
	}

	memset(attr.get_ptr(), 0, attr->size);
	attr->m.revision = revision;
	attr->m.sdkVersion = sdkVersion;
	attr->m.nSpus = nSpus;
	attr->m.spuPriority = spuPriority;
	attr->m.ppuPriority = ppuPriority;
	attr->m.exitIfNoWork = exitIfNoWork;
	return CELL_OK;
}

s64 cellSpursAttributeSetMemoryContainerForSpuThread(vm::ptr<CellSpursAttribute> attr, u32 container)
{
	cellSpurs->Warning("cellSpursAttributeSetMemoryContainerForSpuThread(attr_addr=0x%x, container=%d)", attr.addr(), container);
#ifdef PRX_DEBUG_XXX
	return GetCurrentPPUThread().FastCall2(libsre + 0x6FF8, libsre_rtoc);
#endif

	if (!attr)
	{
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;
	}
	if (attr.addr() % 8)
	{
		return CELL_SPURS_CORE_ERROR_ALIGN;
	}

	if (attr->m.flags & SAF_SPU_TGT_EXCLUSIVE_NON_CONTEXT)
	{
		return CELL_SPURS_CORE_ERROR_STAT;
	}

	attr->m.container = container;
	attr->m.flags |= SAF_SPU_MEMORY_CONTAINER_SET;
	return CELL_OK;
}

s64 cellSpursAttributeSetNamePrefix(vm::ptr<CellSpursAttribute> attr, vm::ptr<const char> prefix, u32 size)
{
	cellSpurs->Warning("cellSpursAttributeSetNamePrefix(attr_addr=0x%x, prefix_addr=0x%x, size=%d)", attr.addr(), prefix.addr(), size);
#ifdef PRX_DEBUG_XXX
	return GetCurrentPPUThread().FastCall2(libsre + 0x7234, libsre_rtoc);
#endif

	if (!attr || !prefix)
	{
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;
	}
	if (attr.addr() % 8)
	{
		return CELL_SPURS_CORE_ERROR_ALIGN;
	}

	if (size > CELL_SPURS_NAME_MAX_LENGTH)
	{
		return CELL_SPURS_CORE_ERROR_INVAL;
	}

	memcpy(attr->m.prefix, prefix.get_ptr(), size);
	attr->m.prefixSize = size;
	return CELL_OK;
}

s64 cellSpursAttributeEnableSpuPrintfIfAvailable(vm::ptr<CellSpursAttribute> attr)
{
	cellSpurs->Warning("cellSpursAttributeEnableSpuPrintfIfAvailable(attr_addr=0x%x)", attr.addr());
#ifdef PRX_DEBUG_XXX
	return GetCurrentPPUThread().FastCall2(libsre + 0x7150, libsre_rtoc);
#endif

	if (!attr)
	{
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;
	}
	if (attr.addr() % 8)
	{
		return CELL_SPURS_CORE_ERROR_ALIGN;
	}

	attr->m.flags |= SAF_SPU_PRINTF_ENABLED;
	return CELL_OK;
}

s64 cellSpursAttributeSetSpuThreadGroupType(vm::ptr<CellSpursAttribute> attr, s32 type)
{
	cellSpurs->Warning("cellSpursAttributeSetSpuThreadGroupType(attr_addr=0x%x, type=%d)", attr.addr(), type);
#ifdef PRX_DEBUG_XXX
	return GetCurrentPPUThread().FastCall2(libsre + 0x70C8, libsre_rtoc);
#endif

	if (!attr)
	{
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;
	}
	if (attr.addr() % 8)
	{
		return CELL_SPURS_CORE_ERROR_ALIGN;
	}

	if (type == SYS_SPU_THREAD_GROUP_TYPE_EXCLUSIVE_NON_CONTEXT)
	{
		if (attr->m.flags & SAF_SPU_MEMORY_CONTAINER_SET)
		{
			return CELL_SPURS_CORE_ERROR_STAT;
		}
		attr->m.flags |= SAF_SPU_TGT_EXCLUSIVE_NON_CONTEXT; // set
	}
	else if (type == SYS_SPU_THREAD_GROUP_TYPE_NORMAL)
	{
		attr->m.flags &= ~SAF_SPU_TGT_EXCLUSIVE_NON_CONTEXT; // clear
	}
	else
	{
		return CELL_SPURS_CORE_ERROR_INVAL;
	}
	return CELL_OK;
}

s64 cellSpursAttributeEnableSystemWorkload(vm::ptr<CellSpursAttribute> attr, vm::ptr<const u8[8]> priority, u32 maxSpu, vm::ptr<const bool[8]> isPreemptible)
{
	cellSpurs->Warning("cellSpursAttributeEnableSystemWorkload(attr_addr=0x%x, priority_addr=0x%x, maxSpu=%d, isPreemptible_addr=0x%x)",
		attr.addr(), priority.addr(), maxSpu, isPreemptible.addr());
#ifdef PRX_DEBUG_XXX
	return GetCurrentPPUThread().FastCall2(libsre + 0xF410, libsre_rtoc);
#endif

	if (!attr)
	{
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;
	}
	if (attr.addr() % 8)
	{
		return CELL_SPURS_CORE_ERROR_ALIGN;
	}

	const u32 nSpus = attr->_u32[2];
	if (!nSpus)
	{
		return CELL_SPURS_CORE_ERROR_INVAL;
	}
	for (u32 i = 0; i < nSpus; i++)
	{
		if ((*priority)[i] == 1)
		{
			if (!maxSpu)
			{
				return CELL_SPURS_CORE_ERROR_INVAL;
			}
			if (nSpus == 1 || attr->m.exitIfNoWork)
			{
				return CELL_SPURS_CORE_ERROR_PERM;
			}
			if (attr->m.flags & SAF_SYSTEM_WORKLOAD_ENABLED)
			{
				return CELL_SPURS_CORE_ERROR_BUSY;
			}

			attr->m.flags |= SAF_SYSTEM_WORKLOAD_ENABLED; // set flag
			*(u64*)attr->m.swlPriority = *(u64*)*priority; // copy system workload priorities

			u32 isPreem = 0; // generate mask from isPreemptible values
			for (u32 j = 0; j < nSpus; j++)
			{
				if ((*isPreemptible)[j])
				{
					isPreem |= (1 << j);
				}
			}
			attr->m.swlMaxSpu = maxSpu; // write max spu for system workload
			attr->m.swlIsPreem = isPreem; // write isPreemptible mask
			return CELL_OK;
		}
	}
	return CELL_SPURS_CORE_ERROR_INVAL;
}

s64 cellSpursFinalize(vm::ptr<CellSpurs> spurs)
{
	cellSpurs->Todo("cellSpursFinalize(spurs_addr=0x%x)", spurs.addr());
#ifdef PRX_DEBUG_XXX
	return GetCurrentPPUThread().FastCall2(libsre + 0x8568, libsre_rtoc);
#endif

	if (!spurs)
	{
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;
	}
	if (spurs.addr() % 128)
	{
		return CELL_SPURS_CORE_ERROR_ALIGN;
	}
	if (spurs->m.xD66.read_relaxed())
	{
		return CELL_SPURS_CORE_ERROR_STAT;
	}



	return CELL_OK;
}

s64 spursAttachLv2EventQueue(vm::ptr<CellSpurs> spurs, u32 queue, vm::ptr<u8> port, s32 isDynamic, bool wasCreated)
{
#ifdef PRX_DEBUG_XXX
	return cb_call<s32, vm::ptr<CellSpurs>, u32, vm::ptr<u8>, s32, bool>(GetCurrentPPUThread(), libsre + 0xAE34, libsre_rtoc,
		spurs, queue, port, isDynamic, wasCreated);
#endif

	if (!spurs || !port)
	{
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;
	}
	if (spurs.addr() % 128)
	{
		return CELL_SPURS_CORE_ERROR_ALIGN;
	}
	if (spurs->m.exception.data())
	{
		return CELL_SPURS_CORE_ERROR_STAT;
	}

	s32 sdk_ver;
	if (s32 res = process_get_sdk_version(process_getpid(), sdk_ver))
	{
		assert(!"process_get_sdk_version() failed");
	}
	if (sdk_ver == -1) sdk_ver = 0x460000;

	u8 _port = 0x3f;
	u64 port_mask = 0;

	if (isDynamic == 0)
	{
		_port = *port;
		if (_port > 0x3f)
		{
			return CELL_SPURS_CORE_ERROR_INVAL;
		}
		if (sdk_ver > 0x17ffff && _port > 0xf)
		{
			return CELL_SPURS_CORE_ERROR_PERM;
		}
	}

	for (u32 i = isDynamic ? 0x10 : _port; i <= _port; i++)
	{
		port_mask |= 1ull << (i);
	}

	assert(port_mask); // zero mask will return CELL_EINVAL
	if (s32 res = sys_spu_thread_group_connect_event_all_threads(spurs->m.spuTG, queue, port_mask, port))
	{
		if (res == CELL_EISCONN)
		{
			return CELL_SPURS_CORE_ERROR_BUSY;
		}
		return res;
	}

	if (!wasCreated)
	{
		spurs->m.spups |= be_t<u64>::make(1ull << *port); // atomic bitwise or
	}
	return CELL_OK;
}

s64 cellSpursAttachLv2EventQueue(vm::ptr<CellSpurs> spurs, u32 queue, vm::ptr<u8> port, s32 isDynamic)
{
	cellSpurs->Warning("cellSpursAttachLv2EventQueue(spurs_addr=0x%x, queue=%d, port_addr=0x%x, isDynamic=%d)",
		spurs.addr(), queue, port.addr(), isDynamic);
#ifdef PRX_DEBUG_XXX
	return GetCurrentPPUThread().FastCall2(libsre + 0xAFE0, libsre_rtoc);
#endif

	return spursAttachLv2EventQueue(spurs, queue, port, isDynamic, false);
}

s64 cellSpursDetachLv2EventQueue(vm::ptr<CellSpurs> spurs, u8 port)
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("cellSpursDetachLv2EventQueue(spurs_addr=0x%x, port=%d)", spurs.addr(), port);
	return GetCurrentPPUThread().FastCall2(libsre + 0xB144, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursGetSpuGuid()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0xEFB0, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursGetSpuThreadGroupId(vm::ptr<CellSpurs> spurs, vm::ptr<u32> group)
{
	cellSpurs->Warning("cellSpursGetSpuThreadGroupId(spurs_addr=0x%x, group_addr=0x%x)", spurs.addr(), group.addr());
#ifdef PRX_DEBUG_XXX
	return GetCurrentPPUThread().FastCall2(libsre + 0x8B30, libsre_rtoc);
#endif

	if (!spurs || !group)
	{
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;
	}
	if (spurs.addr() % 128)
	{
		return CELL_SPURS_CORE_ERROR_ALIGN;
	}

	*group = spurs->m.spuTG;
	return CELL_OK;
}

s64 cellSpursGetNumSpuThread(vm::ptr<CellSpurs> spurs, vm::ptr<u32> nThreads)
{
	cellSpurs->Warning("cellSpursGetNumSpuThread(spurs_addr=0x%x, nThreads_addr=0x%x)", spurs.addr(), nThreads.addr());
#ifdef PRX_DEBUG_XXX
	return GetCurrentPPUThread().FastCall2(libsre + 0x8B78, libsre_rtoc);
#endif

	if (!spurs || !nThreads)
	{
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;
	}
	if (spurs.addr() % 128)
	{
		return CELL_SPURS_CORE_ERROR_ALIGN;
	}

	*nThreads = (u32)spurs->m.nSpus;
	return CELL_OK;
}

s64 cellSpursGetSpuThreadId(vm::ptr<CellSpurs> spurs, vm::ptr<u32> thread, vm::ptr<u32> nThreads)
{
	cellSpurs->Warning("cellSpursGetSpuThreadId(spurs_addr=0x%x, thread_addr=0x%x, nThreads_addr=0x%x)", spurs.addr(), thread.addr(), nThreads.addr());
#ifdef PRX_DEBUG_XXX
	return GetCurrentPPUThread().FastCall2(libsre + 0x8A98, libsre_rtoc);
#endif

	if (!spurs || !thread || !nThreads)
	{
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;
	}
	if (spurs.addr() % 128)
	{
		return CELL_SPURS_CORE_ERROR_ALIGN;
	}

	const u32 count = std::min<u32>(*nThreads, spurs->m.nSpus);
	for (u32 i = 0; i < count; i++)
	{
		thread[i] = spurs->m.spus[i];
	}
	*nThreads = count;
	return CELL_OK;
}

s64 cellSpursSetMaxContention(vm::ptr<CellSpurs> spurs, u32 workloadId, u32 maxContention)
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("cellSpursSetMaxContention(spurs_addr=0x%x, workloadId=%d, maxContention=%d)", spurs.addr(), workloadId, maxContention);
	return GetCurrentPPUThread().FastCall2(libsre + 0x8E90, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursSetPriorities(vm::ptr<CellSpurs> spurs, u32 workloadId, vm::ptr<const u8> priorities)
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("cellSpursSetPriorities(spurs_addr=0x%x, workloadId=%d, priorities_addr=0x%x)", spurs.addr(), workloadId, priorities.addr());
	return GetCurrentPPUThread().FastCall2(libsre + 0x8BC0, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursSetPreemptionVictimHints(vm::ptr<CellSpurs> spurs, vm::ptr<const bool> isPreemptible)
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("cellSpursSetPreemptionVictimHints(spurs_addr=0x%x, isPreemptible_addr=0x%x)", spurs.addr(), isPreemptible.addr());
	return GetCurrentPPUThread().FastCall2(libsre + 0xF5A4, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursEnableExceptionEventHandler(vm::ptr<CellSpurs> spurs, bool flag)
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("cellSpursEnableExceptionEventHandler(spurs_addr=0x%x, flag=%d)", spurs.addr(), flag);
	return GetCurrentPPUThread().FastCall2(libsre + 0xDCC0, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursSetGlobalExceptionEventHandler(vm::ptr<CellSpurs> spurs, u32 eaHandler_addr, u32 arg_addr)
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("cellSpursSetGlobalExceptionEventHandler(spurs_addr=0x%x, eaHandler_addr=0x%x, arg_addr=0x%x)",
		spurs.addr(), eaHandler_addr, arg_addr);
	return GetCurrentPPUThread().FastCall2(libsre + 0xD6D0, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursUnsetGlobalExceptionEventHandler(vm::ptr<CellSpurs> spurs)
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("cellSpursUnsetGlobalExceptionEventHandler(spurs_addr=0x%x)", spurs.addr());
	return GetCurrentPPUThread().FastCall2(libsre + 0xD674, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursGetInfo(vm::ptr<CellSpurs> spurs, vm::ptr<CellSpursInfo> info)
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("cellSpursGetInfo(spurs_addr=0x%x, info_addr=0x%x)", spurs.addr(), info.addr());
	return GetCurrentPPUThread().FastCall2(libsre + 0xE540, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 spursWakeUp(PPUThread& CPU, vm::ptr<CellSpurs> spurs)
{
#ifdef PRX_DEBUG_XXX
	return cb_call<s32, vm::ptr<CellSpurs>>(GetCurrentPPUThread(), libsre + 0x84D8, libsre_rtoc, spurs);
#endif

	if (!spurs)
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_NULL_POINTER;
	}
	if (spurs.addr() % 128)
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_ALIGN;
	}
	if (spurs->m.exception.data())
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_STAT;
	}

	spurs->m.xD64.exchange(1);
	if (spurs->m.xD65.read_sync())
	{
		if (s32 res = sys_lwmutex_lock(CPU, spurs->get_lwmutex(), 0))
		{
			assert(!"sys_lwmutex_lock() failed");
		}
		if (s32 res = sys_lwcond_signal(spurs->get_lwcond()))
		{
			assert(!"sys_lwcond_signal() failed");
		}
		if (s32 res = sys_lwmutex_unlock(CPU, spurs->get_lwmutex()))
		{
			assert(!"sys_lwmutex_unlock() failed");
		}
	}
	return CELL_OK;
}

s64 cellSpursWakeUp(PPUThread& CPU, vm::ptr<CellSpurs> spurs)
{
	cellSpurs->Warning("%s(spurs_addr=0x%x)", __FUNCTION__, spurs.addr());

	return spursWakeUp(CPU, spurs);
}

s32 spursAddWorkload(
	vm::ptr<CellSpurs> spurs,
	vm::ptr<u32> wid,
	vm::ptr<const void> pm,
	u32 size,
	u64 data,
	const u8 priorityTable[],
	u32 minContention,
	u32 maxContention,
	vm::ptr<const char> nameClass,
	vm::ptr<const char> nameInstance,
	vm::ptr<CellSpursShutdownCompletionEventHook> hook,
	vm::ptr<void> hookArg)
{
#ifdef PRX_DEBUG_XXX
	return cb_call<s32, vm::ptr<CellSpurs>, vm::ptr<u32>, vm::ptr<const void>, u32, u64, u32, u32, u32, u32, u32, u32, u32>(GetCurrentPPUThread(), libsre + 0x96EC, libsre_rtoc,
		spurs, wid, pm, size, data, vm::get_addr(priorityTable), minContention, maxContention,
		nameClass.addr(), nameInstance.addr(), hook.addr(), hookArg.addr());
#endif

	if (!spurs || !wid || !pm)
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_NULL_POINTER;
	}
	if (spurs.addr() % 128 || pm.addr() % 16)
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_ALIGN;
	}
	if (minContention == 0 || *(u64*)priorityTable & 0xf0f0f0f0f0f0f0f0ull) // check if some priority > 15
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_INVAL;
	}
	if (spurs->m.exception.data())
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_STAT;
	}
	
	u32 wnum;
	const u32 wmax = spurs->m.flags1 & SF1_32_WORKLOADS ? 0x20u : 0x10u; // TODO: check if can be changed
	spurs->m.wklMskA.atomic_op([spurs, wmax, &wnum](be_t<u32>& value)
	{
		wnum = cntlz32(~(u32)value); // found empty position
		if (wnum < wmax)
		{
			value |= (u32)(0x80000000ull >> wnum); // set workload bit
		}
	});

	*wid = wnum; // store workload id
	if (wnum >= wmax)
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_AGAIN;
	}

	u32 index = wnum & 0xf;
	if (wnum <= 15)
	{
		assert((spurs->m.wklCurrentContention[wnum] & 0xf) == 0);
		assert((spurs->m.wklPendingContention[wnum] & 0xf) == 0);
		spurs->m.wklState1[wnum].write_relaxed(1);
		spurs->m.wklStatus1[wnum] = 0;
		spurs->m.wklEvent1[wnum] = 0;
		spurs->m.wklInfo1[wnum].addr = pm;
		spurs->m.wklInfo1[wnum].arg = data;
		spurs->m.wklInfo1[wnum].size = size;
		for (u32 i = 0; i < 8; i++)
		{
			spurs->m.wklInfo1[wnum].priority[i] = priorityTable[i];
		}
		spurs->m.wklH1[wnum].nameClass = nameClass;
		spurs->m.wklH1[wnum].nameInstance = nameInstance;
		memset(spurs->m.wklF1[wnum].unk0, 0, 0x20); // clear struct preserving semaphore id
		memset(spurs->m.wklF1[wnum].unk1, 0, 0x58);
		if (hook)
		{
			spurs->m.wklF1[wnum].hook = hook;
			spurs->m.wklF1[wnum].hookArg = hookArg;
			spurs->m.wklEvent1[wnum] |= 2;
		}
		if ((spurs->m.flags1 & SF1_32_WORKLOADS) == 0)
		{
			spurs->m.wklIdleSpuCountOrReadyCount2[wnum].write_relaxed(0);
			spurs->m.wklMinContention[wnum] = minContention > 8 ? 8 : minContention;
		}
		spurs->m.wklReadyCount1[wnum].write_relaxed(0);
	}
	else
	{
		assert((spurs->m.wklCurrentContention[index] & 0xf0) == 0);
		assert((spurs->m.wklPendingContention[index] & 0xf0) == 0);
		spurs->m.wklState2[index].write_relaxed(1);
		spurs->m.wklStatus2[index] = 0;
		spurs->m.wklEvent2[index] = 0;
		spurs->m.wklInfo2[index].addr = pm;
		spurs->m.wklInfo2[index].arg = data;
		spurs->m.wklInfo2[index].size = size;
		for (u32 i = 0; i < 8; i++)
		{
			spurs->m.wklInfo2[index].priority[i] = priorityTable[i];
		}
		spurs->m.wklH2[index].nameClass = nameClass;
		spurs->m.wklH2[index].nameInstance = nameInstance;
		memset(spurs->m.wklF2[index].unk0, 0, 0x20); // clear struct preserving semaphore id
		memset(spurs->m.wklF2[index].unk1, 0, 0x58);
		if (hook)
		{
			spurs->m.wklF2[index].hook = hook;
			spurs->m.wklF2[index].hookArg = hookArg;
			spurs->m.wklEvent2[index] |= 2;
		}
		spurs->m.wklIdleSpuCountOrReadyCount2[wnum].write_relaxed(0);
	}

	if (wnum <= 15)
	{
		spurs->m.wklMaxContention[wnum].atomic_op([maxContention](u8& v)
		{
			v &= ~0xf;
			v |= (maxContention > 8 ? 8 : maxContention);
		});
		spurs->m.wklSignal1._and_not({ be_t<u16>::make(0x8000 >> index) }); // clear bit in wklFlag1
	}
	else
	{
		spurs->m.wklMaxContention[index].atomic_op([maxContention](u8& v)
		{
			v &= ~0xf0;
			v |= (maxContention > 8 ? 8 : maxContention) << 4;
		});
		spurs->m.wklSignal2._and_not({ be_t<u16>::make(0x8000 >> index) }); // clear bit in wklFlag2
	}

	spurs->m.wklFlagReceiver.compare_and_swap(wnum, 0xff);

	u32 res_wkl;
	CellSpurs::WorkloadInfo& wkl = wnum <= 15 ? spurs->m.wklInfo1[wnum] : spurs->m.wklInfo2[wnum & 0xf];
	spurs->m.wklMskB.atomic_op_sync([spurs, &wkl, wnum, &res_wkl](be_t<u32>& v)
	{
		const u32 mask = v & ~(0x80000000u >> wnum);
		res_wkl = 0;

		for (u32 i = 0, m = 0x80000000, k = 0; i < 32; i++, m >>= 1)
		{
			if (mask & m)
			{
				CellSpurs::WorkloadInfo& current = i <= 15 ? spurs->m.wklInfo1[i] : spurs->m.wklInfo2[i & 0xf];
				if (current.addr.addr() == wkl.addr.addr())
				{
					// if a workload with identical policy module found
					res_wkl = current.uniqueId.read_relaxed();
					break;
				}
				else
				{
					k |= 0x80000000 >> current.uniqueId.read_relaxed();
					res_wkl = cntlz32(~k);
				}
			}
		}

		wkl.uniqueId.exchange((u8)res_wkl);
		v = mask | (0x80000000u >> wnum);
	});
	assert(res_wkl <= 31);

	spurs->wklState(wnum).exchange(2);
	spurs->m.sysSrvMsgUpdateWorkload.exchange(0xff);
	spurs->m.sysSrvMessage.exchange(0xff);
	return CELL_OK;
}

s64 cellSpursAddWorkload(
	vm::ptr<CellSpurs> spurs,
	vm::ptr<u32> wid,
	vm::ptr<const void> pm,
	u32 size,
	u64 data,
	vm::ptr<const u8[8]> priorityTable,
	u32 minContention,
	u32 maxContention)
{
	cellSpurs->Warning("%s(spurs_addr=0x%x, wid_addr=0x%x, pm_addr=0x%x, size=0x%x, data=0x%llx, priorityTable_addr=0x%x, minContention=0x%x, maxContention=0x%x)",
		__FUNCTION__, spurs.addr(), wid.addr(), pm.addr(), size, data, priorityTable.addr(), minContention, maxContention);
#ifdef PRX_DEBUG_XXX
	return GetCurrentPPUThread().FastCall2(libsre + 0x9ED0, libsre_rtoc);
#endif

	return spursAddWorkload(
		spurs,
		wid,
		pm,
		size,
		data,
		*priorityTable,
		minContention,
		maxContention,
		vm::ptr<const char>::make(0),
		vm::ptr<const char>::make(0),
		vm::ptr<CellSpursShutdownCompletionEventHook>::make(0),
		vm::ptr<void>::make(0));
}

s64 _cellSpursWorkloadAttributeInitialize(
	vm::ptr<CellSpursWorkloadAttribute> attr,
	u32 revision,
	u32 sdkVersion,
	vm::ptr<const void> pm,
	u32 size,
	u64 data,
	vm::ptr<const u8[8]> priorityTable,
	u32 minContention,
	u32 maxContention)
{
	cellSpurs->Warning("%s(attr_addr=0x%x, revision=%d, sdkVersion=0x%x, pm_addr=0x%x, size=0x%x, data=0x%llx, priorityTable_addr=0x%x, minContention=0x%x, maxContention=0x%x)",
		__FUNCTION__, attr.addr(), revision, sdkVersion, pm.addr(), size, data, priorityTable.addr(), minContention, maxContention);
#ifdef PRX_DEBUG_XXX
	return GetCurrentPPUThread().FastCall2(libsre + 0x9F08, libsre_rtoc);
#endif

	if (!attr)
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_NULL_POINTER;
	}
	if (attr.addr() % 8)
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_ALIGN;
	}
	if (!pm)
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_NULL_POINTER;
	}
	if (pm.addr() % 16)
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_ALIGN;
	}
	if (minContention == 0 || *(u64*)*priorityTable & 0xf0f0f0f0f0f0f0f0ull) // check if some priority > 15
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_INVAL;
	}
	
	memset(attr.get_ptr(), 0, CellSpursWorkloadAttribute::size);
	attr->m.revision = revision;
	attr->m.sdkVersion = sdkVersion;
	attr->m.pm = pm;
	attr->m.size = size;
	attr->m.data = data;
	*(u64*)attr->m.priority = *(u64*)*priorityTable;
	attr->m.minContention = minContention;
	attr->m.maxContention = maxContention;
	return CELL_OK;
}

s64 cellSpursWorkloadAttributeSetName(vm::ptr<CellSpursWorkloadAttribute> attr, vm::ptr<const char> nameClass, vm::ptr<const char> nameInstance)
{
	cellSpurs->Warning("%s(attr_addr=0x%x, nameClass_addr=0x%x, nameInstance_addr=0x%x)", __FUNCTION__, attr.addr(), nameClass.addr(), nameInstance.addr());
#ifdef PRX_DEBUG_XXX
	return GetCurrentPPUThread().FastCall2(libsre + 0x9664, libsre_rtoc);
#endif

	if (!attr)
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_NULL_POINTER;
	}
	if (attr.addr() % 8)
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_ALIGN;
	}

	attr->m.nameClass = nameClass;
	attr->m.nameInstance = nameInstance;
	return CELL_OK;
}

s64 cellSpursWorkloadAttributeSetShutdownCompletionEventHook(vm::ptr<CellSpursWorkloadAttribute> attr, vm::ptr<CellSpursShutdownCompletionEventHook> hook, vm::ptr<void> arg)
{
	cellSpurs->Warning("%s(attr_addr=0x%x, hook_addr=0x%x, arg=0x%x)", __FUNCTION__, attr.addr(), hook.addr(), arg.addr());
#ifdef PRX_DEBUG_XXX
	return GetCurrentPPUThread().FastCall2(libsre + 0x96A4, libsre_rtoc);
#endif

	if (!attr || !hook)
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_NULL_POINTER;
	}
	if (attr.addr() % 8)
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_ALIGN;
	}

	attr->m.hook = hook;
	attr->m.hookArg = arg;
	return CELL_OK;
}

s64 cellSpursAddWorkloadWithAttribute(vm::ptr<CellSpurs> spurs, const vm::ptr<u32> wid, vm::ptr<const CellSpursWorkloadAttribute> attr)
{
	cellSpurs->Warning("%s(spurs_addr=0x%x, wid_addr=0x%x, attr_addr=0x%x)", __FUNCTION__, spurs.addr(), wid.addr(), attr.addr());
#ifdef PRX_DEBUG_XXX
	return GetCurrentPPUThread().FastCall2(libsre + 0x9E14, libsre_rtoc);
#endif

	if (!attr)
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_NULL_POINTER;
	}
	if (attr.addr() % 8)
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_ALIGN;
	}
	if (attr->m.revision != be_t<u32>::make(1))
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_INVAL;
	}

	return spursAddWorkload(
		spurs,
		wid,
		vm::ptr<const void>::make(attr->m.pm.addr()),
		attr->m.size,
		attr->m.data,
		attr->m.priority,
		attr->m.minContention,
		attr->m.maxContention,
		vm::ptr<const char>::make(attr->m.nameClass.addr()),
		vm::ptr<const char>::make(attr->m.nameInstance.addr()),
		vm::ptr<CellSpursShutdownCompletionEventHook>::make(attr->m.hook.addr()),
		vm::ptr<void>::make(attr->m.hookArg.addr()));
}

s64 cellSpursRemoveWorkload()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0xA414, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursWaitForWorkloadShutdown()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0xA20C, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursShutdownWorkload()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0xA060, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 _cellSpursWorkloadFlagReceiver(vm::ptr<CellSpurs> spurs, u32 wid, u32 is_set)
{
	cellSpurs->Warning("%s(spurs_addr=0x%x, wid=%d, is_set=%d)", __FUNCTION__, spurs.addr(), wid, is_set);
#ifdef PRX_DEBUG_XXX
	return GetCurrentPPUThread().FastCall2(libsre + 0xF158, libsre_rtoc);
#endif

	if (!spurs)
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_NULL_POINTER;
	}
	if (spurs.addr() % 128)
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_ALIGN;
	}
	if (wid >= (spurs->m.flags1 & SF1_32_WORKLOADS ? 0x20u : 0x10u))
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_INVAL;
	}
	if ((spurs->m.wklMskA.read_relaxed() & (0x80000000u >> wid)) == 0)
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_SRCH;
	}
	if (spurs->m.exception.data())
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_STAT;
	}
	if (s32 res = spurs->m.wklFlag.flag.atomic_op_sync(0, [spurs, wid, is_set](be_t<u32>& flag) -> s32
	{
		if (is_set)
		{
			if (spurs->m.wklFlagReceiver.read_relaxed() != 0xff)
			{
				return CELL_SPURS_POLICY_MODULE_ERROR_BUSY;
			}
		}
		else
		{
			if (spurs->m.wklFlagReceiver.read_relaxed() != wid)
			{
				return CELL_SPURS_POLICY_MODULE_ERROR_PERM;
			}
		}
		flag = -1;
		return 0;
	}))
	{
		return res;
	}

	spurs->m.wklFlagReceiver.atomic_op([wid, is_set](u8& FR)
	{
		if (is_set)
		{
			if (FR == 0xff)
			{
				FR = (u8)wid;
			}
		}
		else
		{
			if (FR == wid)
			{
				FR = 0xff;
			}
		}
	});
	return CELL_OK;
}

s64 cellSpursGetWorkloadFlag(vm::ptr<CellSpurs> spurs, vm::ptr<vm::bptr<CellSpursWorkloadFlag>> flag)
{
	cellSpurs->Warning("%s(spurs_addr=0x%x, flag_addr=0x%x)", __FUNCTION__, spurs.addr(), flag.addr());
#ifdef PRX_DEBUG_XXX
	return GetCurrentPPUThread().FastCall2(libsre + 0xEC00, libsre_rtoc);
#endif

	if (!spurs || !flag)
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_NULL_POINTER;
	}
	if (spurs.addr() % 128)
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_ALIGN;
	}

	flag->set(vm::get_addr(&spurs->m.wklFlag));
	return CELL_OK;
}

s64 cellSpursSendWorkloadSignal(vm::ptr<CellSpurs> spurs, u32 workloadId)
{
	cellSpurs->Warning("%s(spurs=0x%x, workloadId=0x%x)", __FUNCTION__, spurs.addr(), workloadId);

#ifdef PRX_DEBUG
	return GetCurrentPPUThread().FastCall2(libsre + 0xA658, libsre_rtoc);
#else
	if (!spurs)
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_NULL_POINTER;
	}

	if (spurs.addr() %  CellSpurs::align)
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_ALIGN;
	}

	if (workloadId >= CELL_SPURS_MAX_WORKLOAD2 || (workloadId >= CELL_SPURS_MAX_WORKLOAD && (spurs->m.flags1 & SF1_32_WORKLOADS) == 0))
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_INVAL;
	}

	if ((spurs->m.wklMskA.read_relaxed() & (0x80000000u >> workloadId)) == 0)
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_SRCH;
	}

	if (spurs->m.exception)
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_STAT;
	}

	u8 state;
	if (workloadId >= CELL_SPURS_MAX_WORKLOAD)
	{
		state = spurs->m.wklState2[workloadId & 0x0F].read_relaxed();
	}
	else
	{
		state = spurs->m.wklState1[workloadId].read_relaxed();
	}

	if (state != SPURS_WKL_STATE_RUNNABLE)
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_STAT;
	}

	if (workloadId >= CELL_SPURS_MAX_WORKLOAD)
	{
		spurs->m.wklSignal2 |= be_t<u16>::make(0x8000 >> (workloadId & 0x0F));
	}
	else
	{
		spurs->m.wklSignal1 |= be_t<u16>::make(0x8000 >> workloadId);
	}

	return CELL_OK;
#endif
}

s64 cellSpursGetWorkloadData()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0xA78C, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursReadyCountStore(vm::ptr<CellSpurs> spurs, u32 wid, u32 value)
{
	cellSpurs->Warning("%s(spurs_addr=0x%x, wid=%d, value=0x%x)", __FUNCTION__, spurs.addr(), wid, value);
#ifdef PRX_DEBUG_XXX
	return GetCurrentPPUThread().FastCall2(libsre + 0xAB2C, libsre_rtoc);
#endif

	if (!spurs)
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_NULL_POINTER;
	}
	if (spurs.addr() % 128)
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_ALIGN;
	}
	if (wid >= (spurs->m.flags1 & SF1_32_WORKLOADS ? 0x20u : 0x10u) || value > 0xff)
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_INVAL;
	}
	if ((spurs->m.wklMskA.read_relaxed() & (0x80000000u >> wid)) == 0)
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_SRCH;
	}
	if (spurs->m.exception.data() || spurs->wklState(wid).read_relaxed() != 2)
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_STAT;
	}

	if (wid < CELL_SPURS_MAX_WORKLOAD)
	{
		spurs->m.wklReadyCount1[wid].exchange((u8)value);
	}
	else
	{
		spurs->m.wklIdleSpuCountOrReadyCount2[wid].exchange((u8)value);
	}
	return CELL_OK;
}

s64 cellSpursReadyCountAdd()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0xA868, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursReadyCountCompareAndSwap()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0xA9CC, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursReadyCountSwap()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0xAC34, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursRequestIdleSpu()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0xAD88, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursGetWorkloadInfo()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0xE70C, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 _cellSpursWorkloadFlagReceiver2()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0xF298, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursSetExceptionEventHandler()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0xDB54, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursUnsetExceptionEventHandler()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0xD77C, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 _cellSpursEventFlagInitialize(vm::ptr<CellSpurs> spurs, vm::ptr<CellSpursTaskset> taskset, vm::ptr<CellSpursEventFlag> eventFlag, u32 flagClearMode, u32 flagDirection)
{
	cellSpurs->Warning("_cellSpursEventFlagInitialize(spurs_addr=0x%x, taskset_addr=0x%x, eventFlag_addr=0x%x, flagClearMode=%d, flagDirection=%d)",
		spurs.addr(), taskset.addr(), eventFlag.addr(), flagClearMode, flagDirection);

#ifdef PRX_DEBUG
	return GetCurrentPPUThread().FastCall2(libsre + 0x1564C, libsre_rtoc);
#else
	if (taskset.addr() == 0 || spurs.addr() == 0)
	{
		return CELL_SPURS_TASK_ERROR_NULL_POINTER;
	}

	if (eventFlag.addr() == 0)
	{
		return CELL_SPURS_TASK_ERROR_NULL_POINTER;
	}

	if (spurs.addr() % CellSpurs::align || taskset.addr() % CellSpursTaskset::align || eventFlag.addr() % CellSpursEventFlag::align)
	{
		return CELL_SPURS_TASK_ERROR_ALIGN;
	}

	if (taskset.addr() && taskset->m.wid >= CELL_SPURS_MAX_WORKLOAD2)
	{
		return CELL_SPURS_TASK_ERROR_INVAL;
	}

	if (flagDirection > CELL_SPURS_EVENT_FLAG_LAST || flagClearMode > CELL_SPURS_EVENT_FLAG_CLEAR_LAST)
	{
		return CELL_SPURS_TASK_ERROR_INVAL;
	}

	memset(eventFlag.get_ptr(), 0, CellSpursEventFlag::size);
	eventFlag->m.direction = flagDirection;
	eventFlag->m.clearMode = flagClearMode;
	eventFlag->m.spuPort   = CELL_SPURS_EVENT_FLAG_INVALID_SPU_PORT;

	if (taskset.addr())
	{
		eventFlag->m.addr = taskset.addr();
	}
	else
	{
		eventFlag->m.isIwl = 1;
		eventFlag->m.addr  = spurs.addr();
	}

	return CELL_OK;
#endif
}

s64 cellSpursEventFlagAttachLv2EventQueue(vm::ptr<CellSpursEventFlag> eventFlag)
{
	cellSpurs->Warning("cellSpursEventFlagAttachLv2EventQueue(eventFlag_addr=0x%x)", eventFlag.addr());

#ifdef PRX_DEBUG
	return GetCurrentPPUThread().FastCall2(libsre + 0x157B8, libsre_rtoc);
#else
	if (!eventFlag)
	{
		return CELL_SPURS_TASK_ERROR_NULL_POINTER;
	}

	if (eventFlag.addr() % CellSpursEventFlag::align)
	{
		return CELL_SPURS_TASK_ERROR_AGAIN;
	}

	if (eventFlag->m.direction != CELL_SPURS_EVENT_FLAG_SPU2PPU && eventFlag->m.direction != CELL_SPURS_EVENT_FLAG_ANY2ANY)
	{
		return CELL_SPURS_TASK_ERROR_PERM;
	}

	if (eventFlag->m.spuPort != CELL_SPURS_EVENT_FLAG_INVALID_SPU_PORT)
	{
		return CELL_SPURS_TASK_ERROR_STAT;
	}

	vm::ptr<CellSpurs> spurs;
	if (eventFlag->m.isIwl == 1)
	{
		spurs.set((u32)eventFlag->m.addr);
	}
	else
	{
		auto taskset = vm::ptr<CellSpursTaskset>::make((u32)eventFlag->m.addr);
		spurs.set((u32)taskset->m.spurs.addr());
	}

	u32 eventQueueId;
	vm::var<u8> port;
	auto rc = spursCreateLv2EventQueue(spurs, eventQueueId, port, 1, *((u64 *)"_spuEvF"));
	if (rc != CELL_OK)
	{
		// Return rc if its an error code from SPURS otherwise convert the error code to a SPURS task error code
		return (rc & 0x0FFF0000) == 0x00410000 ? rc : (0x80410900 | (rc & 0xFF));
	}

	if (eventFlag->m.direction == CELL_SPURS_EVENT_FLAG_ANY2ANY)
	{
		vm::var<be_t<u32>> eventPortId;
		rc = sys_event_port_create(vm::ptr<u32>::make(eventPortId.addr()), SYS_EVENT_PORT_LOCAL, 0);
		if (rc == CELL_OK)
		{
			rc = sys_event_port_connect_local(eventPortId.value(), eventQueueId);
			if (rc == CELL_OK)
			{
				eventFlag->m.eventPortId = eventPortId;
				goto success;
			}

			sys_event_port_destroy(eventPortId.value());
		}

		// TODO: Implement the following
		// if (spursDetachLv2EventQueue(spurs, port, 1) == CELL_OK)
		// {
		//     sys_event_queue_destroy(eventQueueId, SYS_EVENT_QUEUE_DESTROY_FORCE);
		// }

		// Return rc if its an error code from SPURS otherwise convert the error code to a SPURS task error code
		return (rc & 0x0FFF0000) == 0x00410000 ? rc : (0x80410900 | (rc & 0xFF));
	}

success:
	eventFlag->m.eventQueueId = eventQueueId;
	eventFlag->m.spuPort      = port;
	return CELL_OK;
#endif
}

s64 cellSpursEventFlagDetachLv2EventQueue(vm::ptr<CellSpursEventFlag> eventFlag)
{
	cellSpurs->Warning("cellSpursEventFlagDetachLv2EventQueue(eventFlag_addr=0x%x)", eventFlag.addr());

#ifdef PRX_DEBUG
	return GetCurrentPPUThread().FastCall2(libsre + 0x15998, libsre_rtoc);
#else
	if (!eventFlag)
	{
		return CELL_SPURS_TASK_ERROR_NULL_POINTER;
	}

	if (eventFlag.addr() % CellSpursEventFlag::align)
	{
		return CELL_SPURS_TASK_ERROR_AGAIN;
	}

	if (eventFlag->m.direction != CELL_SPURS_EVENT_FLAG_SPU2PPU && eventFlag->m.direction != CELL_SPURS_EVENT_FLAG_ANY2ANY)
	{
		return CELL_SPURS_TASK_ERROR_PERM;
	}

	if (eventFlag->m.spuPort == CELL_SPURS_EVENT_FLAG_INVALID_SPU_PORT)
	{
		return CELL_SPURS_TASK_ERROR_STAT;
	}

	if (eventFlag->m.ppuWaitMask || eventFlag->m.ppuPendingRecv)
	{
		return CELL_SPURS_TASK_ERROR_BUSY;
	}

	auto port            = eventFlag->m.spuPort;
	eventFlag->m.spuPort = CELL_SPURS_EVENT_FLAG_INVALID_SPU_PORT;

	vm::ptr<CellSpurs> spurs;
	if (eventFlag->m.isIwl == 1)
	{
		spurs.set((u32)eventFlag->m.addr);
	}
	else
	{
		auto taskset = vm::ptr<CellSpursTaskset>::make((u32)eventFlag->m.addr);
		spurs.set((u32)taskset->m.spurs.addr());
	}

	if(eventFlag->m.direction == CELL_SPURS_EVENT_FLAG_ANY2ANY)
	{
		sys_event_port_disconnect(eventFlag->m.eventPortId);
		sys_event_port_destroy(eventFlag->m.eventPortId);
	}

	s64 rc = CELL_OK;
	// TODO: Implement the following
	// auto rc = spursDetachLv2EventQueue(spurs, port, 1);
	// if (rc == CELL_OK)
	// {
	//     rc = sys_event_queue_destroy(eventFlag->m.eventQueueId, SYS_EVENT_QUEUE_DESTROY_FORCE);
	// }

	if (rc != CELL_OK)
	{
		// Return rc if its an error code from SPURS otherwise convert the error code to a SPURS task error code
		return (rc & 0x0FFF0000) == 0x00410000 ? rc : (0x80410900 | (rc & 0xFF));
	}

	return CELL_OK;
#endif
}

s64 _cellSpursEventFlagWait(vm::ptr<CellSpursEventFlag> eventFlag, vm::ptr<u16> mask, u32 mode, u32 block)
{
	if (eventFlag.addr() == 0 || mask.addr() == 0)
	{
		return CELL_SPURS_TASK_ERROR_NULL_POINTER;
	}

	if (eventFlag.addr() % CellSpursEventFlag::align)
	{
		return CELL_SPURS_TASK_ERROR_ALIGN;
	}

	if (mode > CELL_SPURS_EVENT_FLAG_WAIT_MODE_LAST)
	{
		return CELL_SPURS_TASK_ERROR_INVAL;
	}

	if (eventFlag->m.direction != CELL_SPURS_EVENT_FLAG_SPU2PPU && eventFlag->m.direction != CELL_SPURS_EVENT_FLAG_ANY2ANY)
	{
		return CELL_SPURS_TASK_ERROR_PERM;
	}

	if (block && eventFlag->m.spuPort == CELL_SPURS_EVENT_FLAG_INVALID_SPU_PORT)
	{
		return CELL_SPURS_TASK_ERROR_STAT;
	}

	if (eventFlag->m.ppuWaitMask || eventFlag->m.ppuPendingRecv)
	{
		return CELL_SPURS_TASK_ERROR_BUSY;
	}

	u16 relevantEvents = eventFlag->m.events & *mask;
	if (eventFlag->m.direction == CELL_SPURS_EVENT_FLAG_ANY2ANY)
	{
		// Make sure the wait mask and mode specified does not conflict with that of the already waiting tasks.
		// Conflict scenarios:
		// OR  vs OR  - A conflict never occurs
		// OR  vs AND - A conflict occurs if the masks for the two tasks overlap
		// AND vs AND - A conflict occurs if the masks for the two tasks are not the same

		// Determine the set of all already waiting tasks whose wait mode/mask can possibly conflict with the specified wait mode/mask.
		// This set is equal to 'set of all tasks waiting' - 'set of all tasks whose wait conditions have been met'.
		// If the wait mode is OR, we prune the set of all tasks that are waiting in OR mode from the set since a conflict cannot occur
		// with an already waiting task in OR mode.
		u16 relevantWaitSlots = eventFlag->m.spuTaskUsedWaitSlots & ~eventFlag->m.spuTaskPendingRecv;
		if (mode == CELL_SPURS_EVENT_FLAG_OR)
		{
			relevantWaitSlots &= eventFlag->m.spuTaskWaitMode;
		}

		int i = CELL_SPURS_EVENT_FLAG_MAX_WAIT_SLOTS - 1;
		while (relevantWaitSlots)
		{
			if (relevantWaitSlots & 0x0001)
			{
				if (eventFlag->m.spuTaskWaitMask[i] & *mask && eventFlag->m.spuTaskWaitMask[i] != *mask)
				{
					return CELL_SPURS_TASK_ERROR_AGAIN;
				}
			}

			relevantWaitSlots >>= 1;
			i--;
		}
	}

	// There is no need to block if all bits required by the wait operation have already been set or
	// if the wait mode is OR and atleast one of the bits required by the wait operation has been set.
	bool recv;
	if ((*mask & ~relevantEvents) == 0 || (mode == CELL_SPURS_EVENT_FLAG_OR && relevantEvents))
	{
		// If the clear flag is AUTO then clear the bits comnsumed by this thread
		if (eventFlag->m.clearMode == CELL_SPURS_EVENT_FLAG_CLEAR_AUTO)
		{
			eventFlag->m.events &= ~relevantEvents;
		}

		recv = false;
	}
	else
	{
		// If we reach here it means that the conditions for this thread have not been met.
		// If this is a try wait operation then do not block but return an error code.
		if (block == 0)
		{
			return CELL_SPURS_TASK_ERROR_BUSY;
		}

		eventFlag->m.ppuWaitSlotAndMode = 0;
		if (eventFlag->m.direction == CELL_SPURS_EVENT_FLAG_ANY2ANY)
		{
			// Find an unsed wait slot
			int i                    = 0;
			u16 spuTaskUsedWaitSlots = eventFlag->m.spuTaskUsedWaitSlots;
			while (spuTaskUsedWaitSlots & 0x0001)
			{
				spuTaskUsedWaitSlots >>= 1;
				i++;
			}

			if (i == CELL_SPURS_EVENT_FLAG_MAX_WAIT_SLOTS)
			{
				// Event flag has no empty wait slots
				return CELL_SPURS_TASK_ERROR_BUSY;
			}

			// Mark the found wait slot as used by this thread
			eventFlag->m.ppuWaitSlotAndMode = (CELL_SPURS_EVENT_FLAG_MAX_WAIT_SLOTS - 1 - i) << 4;
		}

		// Save the wait mask and mode for this thread
		eventFlag->m.ppuWaitSlotAndMode |= mode;
		eventFlag->m.ppuWaitMask         = *mask;
		recv                             = true;
	}

	u16 receivedEventFlag;
	if (recv) {
		// Block till something happens
		vm::var<sys_event_data> data;
		auto rc = sys_event_queue_receive(eventFlag->m.eventQueueId, data, 0);
		if (rc != CELL_OK)
		{
			assert(0);
		}

		int i = 0;
		if (eventFlag->m.direction == CELL_SPURS_EVENT_FLAG_ANY2ANY)
		{
			i = eventFlag->m.ppuWaitSlotAndMode >> 4;
		}

		receivedEventFlag           = eventFlag->m.pendingRecvTaskEvents[i];
		eventFlag->m.ppuPendingRecv = 0;
	}

	*mask = receivedEventFlag;
	return CELL_OK;
}

s64 cellSpursEventFlagWait(vm::ptr<CellSpursEventFlag> eventFlag, vm::ptr<u16> mask, u32 mode)
{
	cellSpurs->Warning("cellSpursEventFlagWait(eventFlag_addr=0x%x, mask_addr=0x%x, mode=%d)", eventFlag.addr(), mask.addr(), mode);

#ifdef PRX_DEBUG
	return GetCurrentPPUThread().FastCall2(libsre + 0x15E68, libsre_rtoc);
#else
	return _cellSpursEventFlagWait(eventFlag, mask, mode, 1/*block*/);
#endif
}

s64 cellSpursEventFlagClear(vm::ptr<CellSpursEventFlag> eventFlag, u16 bits)
{
	cellSpurs->Warning("cellSpursEventFlagClear(eventFlag_addr=0x%x, bits=0x%x)", eventFlag.addr(), bits);

#ifdef PRX_DEBUG
	return GetCurrentPPUThread().FastCall2(libsre + 0x15E9C, libsre_rtoc);
#else
	if (eventFlag.addr() == 0)
	{
		return CELL_SPURS_TASK_ERROR_NULL_POINTER;
	}

	if (eventFlag.addr() % CellSpursEventFlag::align)
	{
		return CELL_SPURS_TASK_ERROR_ALIGN;
	}

	eventFlag->m.events &= ~bits;
	return CELL_OK;
#endif
}

s64 cellSpursEventFlagSet(vm::ptr<CellSpursEventFlag> eventFlag, u16 bits)
{
	cellSpurs->Warning("cellSpursEventFlagSet(eventFlag_addr=0x%x, bits=0x%x)", eventFlag.addr(), bits);

#ifdef PRX_DEBUG
	return GetCurrentPPUThread().FastCall2(libsre + 0x15F04, libsre_rtoc);
#else
	if (eventFlag.addr() == 0)
	{
		return CELL_SPURS_TASK_ERROR_NULL_POINTER;
	}

	if (eventFlag.addr() % CellSpursEventFlag::align)
	{
		return CELL_SPURS_TASK_ERROR_ALIGN;
	}

	if (eventFlag->m.direction != CELL_SPURS_EVENT_FLAG_PPU2SPU && eventFlag->m.direction != CELL_SPURS_EVENT_FLAG_ANY2ANY)
	{
		return CELL_SPURS_TASK_ERROR_PERM;
	}

	u16 ppuEventFlag  = 0;
	bool send         = false;
	int ppuWaitSlot   = 0;
	u16 eventsToClear = 0;
	if (eventFlag->m.direction == CELL_SPURS_EVENT_FLAG_ANY2ANY && eventFlag->m.ppuWaitMask)
	{
		u16 ppuRelevantEvents = (eventFlag->m.events | bits) & eventFlag->m.ppuWaitMask;

		// Unblock the waiting PPU thread if either all the bits being waited by the thread have been set or
		// if the wait mode of the thread is OR and atleast one bit the thread is waiting on has been set
		if ((eventFlag->m.ppuWaitMask & ~ppuRelevantEvents) == 0 ||
			((eventFlag->m.ppuWaitSlotAndMode & 0x0F) == CELL_SPURS_EVENT_FLAG_OR && ppuRelevantEvents != 0))
		{
			eventFlag->m.ppuPendingRecv = 1;
			eventFlag->m.ppuWaitMask    = 0;
			ppuEventFlag                = ppuRelevantEvents;
			eventsToClear               = ppuRelevantEvents;
			ppuWaitSlot                 = eventFlag->m.ppuWaitSlotAndMode >> 4;
			send                        = true;
		}
	}

	int i                  = CELL_SPURS_EVENT_FLAG_MAX_WAIT_SLOTS - 1;
	int j                  = 0;
	u16 relevantWaitSlots  = eventFlag->m.spuTaskUsedWaitSlots & ~eventFlag->m.spuTaskPendingRecv;
	u16 spuTaskPendingRecv = 0;
	u16 pendingRecvTaskEvents[16];
	while (relevantWaitSlots)
	{
		if (relevantWaitSlots & 0x0001)
		{
			u16 spuTaskRelevantEvents = (eventFlag->m.events | bits) & eventFlag->m.spuTaskWaitMask[i];

			// Unblock the waiting SPU task if either all the bits being waited by the task have been set or
			// if the wait mode of the task is OR and atleast one bit the thread is waiting on has been set
			if ((eventFlag->m.spuTaskWaitMask[i] & ~spuTaskRelevantEvents) == 0 || 
				(((eventFlag->m.spuTaskWaitMode >> j) & 0x0001) == CELL_SPURS_EVENT_FLAG_OR && spuTaskRelevantEvents != 0))
			{
				eventsToClear            |= spuTaskRelevantEvents;
				spuTaskPendingRecv       |= 1 << j;
				pendingRecvTaskEvents[j]  = spuTaskRelevantEvents;
			}
		}

		relevantWaitSlots >>= 1;
		i--;
		j++;
	}

	eventFlag->m.events             |= bits;
	eventFlag->m.spuTaskPendingRecv |= spuTaskPendingRecv;

	// If the clear flag is AUTO then clear the bits comnsumed by all tasks marked to be unblocked
	if (eventFlag->m.clearMode == CELL_SPURS_EVENT_FLAG_CLEAR_AUTO)
	{
		 eventFlag->m.events &= ~eventsToClear;
	}

	if (send)
	{
		// Signal the PPU thread to be woken up
		eventFlag->m.pendingRecvTaskEvents[ppuWaitSlot] = ppuEventFlag;
		if (sys_event_port_send(eventFlag->m.eventPortId, 0, 0, 0) != CELL_OK)
		{
			assert(0);
		}
	}

	if (spuTaskPendingRecv)
	{
		// Signal each SPU task whose conditions have been met to be woken up
		for (int i = 0; i < CELL_SPURS_EVENT_FLAG_MAX_WAIT_SLOTS; i++)
		{
			if (spuTaskPendingRecv & (0x8000 >> i))
			{
				eventFlag->m.pendingRecvTaskEvents[i] = pendingRecvTaskEvents[i];
				vm::var<u32> taskset;
				if (eventFlag->m.isIwl)
				{
					cellSpursLookUpTasksetAddress(vm::ptr<CellSpurs>::make((u32)eventFlag->m.addr),
												  vm::ptr<CellSpursTaskset>::make(taskset.addr()),
												  eventFlag->m.waitingTaskWklId[i]);
				}
				else
				{
					taskset.value() = (u32)eventFlag->m.addr;
				}

				auto rc = _cellSpursSendSignal(vm::ptr<CellSpursTaskset>::make(taskset.addr()), eventFlag->m.waitingTaskId[i]);
				if (rc == CELL_SPURS_TASK_ERROR_INVAL || rc == CELL_SPURS_TASK_ERROR_STAT)
				{
					return CELL_SPURS_TASK_ERROR_FATAL;
				}

				if (rc != CELL_OK)
				{
					assert(0);
				}
			}
		}
	}

	return CELL_OK;
#endif
}

s64 cellSpursEventFlagTryWait(vm::ptr<CellSpursEventFlag> eventFlag, vm::ptr<u16> mask, u32 mode)
{
	cellSpurs->Warning("cellSpursEventFlagTryWait(eventFlag_addr=0x%x, mask_addr=0x%x, mode=0x%x)", eventFlag.addr(), mask.addr(), mode);

#ifdef PRX_DEBUG
	return GetCurrentPPUThread().FastCall2(libsre + 0x15E70, libsre_rtoc);
#else
	return _cellSpursEventFlagWait(eventFlag, mask, mode, 0/*block*/);
#endif
}

s64 cellSpursEventFlagGetDirection(vm::ptr<CellSpursEventFlag> eventFlag, vm::ptr<u32> direction)
{
	cellSpurs->Warning("cellSpursEventFlagGetDirection(eventFlag_addr=0x%x, direction_addr=0x%x)", eventFlag.addr(), direction.addr());

#ifdef PRX_DEBUG
	return GetCurrentPPUThread().FastCall2(libsre + 0x162C4, libsre_rtoc);
#else
	if (eventFlag.addr() == 0 || direction.addr() == 0)
	{
		return CELL_SPURS_TASK_ERROR_NULL_POINTER;
	}

	if (eventFlag.addr() % CellSpursEventFlag::align)
	{
		return CELL_SPURS_TASK_ERROR_ALIGN;
	}

	*direction = eventFlag->m.direction;
	return CELL_OK;
#endif
}

s64 cellSpursEventFlagGetClearMode(vm::ptr<CellSpursEventFlag> eventFlag, vm::ptr<u32> clear_mode)
{
	cellSpurs->Warning("cellSpursEventFlagGetClearMode(eventFlag_addr=0x%x, clear_mode_addr=0x%x)", eventFlag.addr(), clear_mode.addr());

#ifdef PRX_DEBUG
	return GetCurrentPPUThread().FastCall2(libsre + 0x16310, libsre_rtoc);
#else
	if (eventFlag.addr() == 0 || clear_mode.addr() == 0)
	{
		return CELL_SPURS_TASK_ERROR_NULL_POINTER;
	}

	if (eventFlag.addr() % CellSpursEventFlag::align)
	{
		return CELL_SPURS_TASK_ERROR_ALIGN;
	}

	*clear_mode = eventFlag->m.clearMode;
	return CELL_OK;
#endif
}

s64 cellSpursEventFlagGetTasksetAddress(vm::ptr<CellSpursEventFlag> eventFlag, vm::ptr<CellSpursTaskset> taskset)
{
	cellSpurs->Warning("cellSpursEventFlagGetTasksetAddress(eventFlag_addr=0x%x, taskset_addr=0x%x)", eventFlag.addr(), taskset.addr());

#ifdef PRX_DEBUG
	return GetCurrentPPUThread().FastCall2(libsre + 0x1635C, libsre_rtoc);
#else
	if (eventFlag.addr() == 0 || taskset.addr() == 0)
	{
		return CELL_SPURS_TASK_ERROR_NULL_POINTER;
	}

	if (eventFlag.addr() % CellSpursEventFlag::align)
	{
		return CELL_SPURS_TASK_ERROR_ALIGN;
	}

	taskset.set(eventFlag->m.isIwl ? 0 : eventFlag->m.addr);
	return CELL_OK;
#endif
}

s64 _cellSpursLFQueueInitialize()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x17028, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 _cellSpursLFQueuePushBody()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x170AC, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursLFQueueDetachLv2EventQueue()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x177CC, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursLFQueueAttachLv2EventQueue()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x173EC, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 _cellSpursLFQueuePopBody()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x17238, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursLFQueueGetTasksetAddress()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x17C34, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 _cellSpursQueueInitialize()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x163B4, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursQueuePopBody()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x16BF0, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursQueuePushBody()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x168C4, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursQueueAttachLv2EventQueue()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x1666C, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursQueueDetachLv2EventQueue()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x16524, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursQueueGetTasksetAddress()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x16F50, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursQueueClear()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x1675C, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursQueueDepth()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x1687C, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursQueueGetEntrySize()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x16FE0, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursQueueSize()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x167F0, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursQueueGetDirection()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x16F98, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursCreateJobChainWithAttribute()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x1898C, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursCreateJobChain()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x18B84, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursJoinJobChain()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x18DB0, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursKickJobChain()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x18E8C, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 _cellSpursJobChainAttributeInitialize()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x1845C, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursGetJobChainId()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x19064, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursJobChainSetExceptionEventHandler()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x1A5A0, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursJobChainUnsetExceptionEventHandler()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x1A614, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursGetJobChainInfo()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x1A7A0, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursJobChainGetSpursAddress()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x1A900, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 spursCreateTaskset(vm::ptr<CellSpurs> spurs, vm::ptr<CellSpursTaskset> taskset, u64 args, vm::ptr<const u8[8]> priority,
	u32 max_contention, vm::ptr<const char> name, u32 size, s32 enable_clear_ls)
{
	if (!spurs || !taskset)
	{
		return CELL_SPURS_TASK_ERROR_NULL_POINTER;
	}

	if (spurs.addr() % CellSpurs::align || taskset.addr() % CellSpursTaskset::align)
	{
		return CELL_SPURS_TASK_ERROR_ALIGN;
	}

	memset(taskset.get_ptr(), 0, size);

	taskset->m.spurs = spurs;
	taskset->m.args = args;
	taskset->m.enable_clear_ls = enable_clear_ls > 0 ? 1 : 0;
	taskset->m.size = size;

	vm::var<CellSpursWorkloadAttribute> wkl_attr;
	_cellSpursWorkloadAttributeInitialize(wkl_attr, 1 /*revision*/, 0x33 /*sdk_version*/, vm::ptr<const void>::make(16) /*pm*/, 0x1E40 /*pm_size*/,
		taskset.addr(), priority, 8 /*min_contention*/, max_contention);
	// TODO: Check return code

	cellSpursWorkloadAttributeSetName(wkl_attr, vm::ptr<const char>::make(0), name);
	// TODO: Check return code

	// TODO: cellSpursWorkloadAttributeSetShutdownCompletionEventHook(wkl_attr, hook, taskset);
	// TODO: Check return code

	vm::var<be_t<u32>> wid;
	cellSpursAddWorkloadWithAttribute(spurs, vm::ptr<u32>::make(wid.addr()), vm::ptr<const CellSpursWorkloadAttribute>::make(wkl_attr.addr()));
	// TODO: Check return code

	taskset->m.x72 = 0x80;
	taskset->m.wid = wid.value();
	// TODO: cellSpursSetExceptionEventHandler(spurs, wid, hook, taskset);
	// TODO: Check return code

	return CELL_OK;
}

s64 cellSpursCreateTasksetWithAttribute(vm::ptr<CellSpurs> spurs, vm::ptr<CellSpursTaskset> taskset, vm::ptr<CellSpursTasksetAttribute> attr)
{
	cellSpurs->Warning("%s(spurs=0x%x, taskset=0x%x, attr=0x%x)", __FUNCTION__, spurs.addr(), taskset.addr(), attr.addr());

#ifdef PRX_DEBUG
	return GetCurrentPPUThread().FastCall2(libsre + 0x14BEC, libsre_rtoc);
#endif

	if (!attr)
	{
		CELL_SPURS_TASK_ERROR_NULL_POINTER;
	}

	if (attr.addr() % CellSpursTasksetAttribute::align)
	{
		return CELL_SPURS_TASK_ERROR_ALIGN;
	}

	if (attr->m.revision != CELL_SPURS_TASKSET_ATTRIBUTE_REVISION)
	{
		return CELL_SPURS_TASK_ERROR_INVAL;
	}

	auto rc = spursCreateTaskset(spurs, taskset, attr->m.args, vm::ptr<const u8[8]>::make(attr.addr() + offsetof(CellSpursTasksetAttribute, m.priority)),
		attr->m.max_contention, vm::ptr<const char>::make(attr->m.name.addr()), attr->m.taskset_size, attr->m.enable_clear_ls);

	if (attr->m.taskset_size >= CellSpursTaskset2::size)
	{
		// TODO: Implement this
	}

	return rc;
}

s64 cellSpursCreateTaskset(vm::ptr<CellSpurs> spurs, vm::ptr<CellSpursTaskset> taskset, u64 args, vm::ptr<const u8[8]> priority, u32 maxContention)
{
	cellSpurs->Warning("cellSpursCreateTaskset(spurs_addr=0x%x, taskset_addr=0x%x, args=0x%llx, priority_addr=0x%x, maxContention=%d)",
		spurs.addr(), taskset.addr(), args, priority.addr(), maxContention);

#ifdef PRX_DEBUG
	return GetCurrentPPUThread().FastCall2(libsre + 0x14CB8, libsre_rtoc);
#endif

#if 0
	SPURSManagerTasksetAttribute *tattr = new SPURSManagerTasksetAttribute(args, priority, maxContention);
	taskset->taskset = new SPURSManagerTaskset(taskset.addr(), tattr);

	return CELL_OK;
#endif

	return spursCreateTaskset(spurs, taskset, args, priority, maxContention, vm::ptr<const char>::make(0), CellSpursTaskset::size, 0);
}

s64 cellSpursJoinTaskset(vm::ptr<CellSpursTaskset> taskset)
{
	cellSpurs->Warning("cellSpursJoinTaskset(taskset_addr=0x%x)", taskset.addr());

#ifdef PRX_DEBUG
	return GetCurrentPPUThread().FastCall2(libsre + 0x152F8, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursGetTasksetId(vm::ptr<CellSpursTaskset> taskset, vm::ptr<u32> wid)
{
	cellSpurs->Warning("cellSpursGetTasksetId(taskset_addr=0x%x, wid=0x%x)", taskset.addr(), wid.addr());

#ifdef PRX_DEBUG
	return GetCurrentPPUThread().FastCall2(libsre + 0x14EA0, libsre_rtoc);
#else
	if (!taskset || !wid)
	{
		return CELL_SPURS_TASK_ERROR_NULL_POINTER;
	}

	if (taskset.addr() % CellSpursTaskset::align)
	{
		return CELL_SPURS_TASK_ERROR_ALIGN;
	}

	if (taskset->m.wid >= CELL_SPURS_MAX_WORKLOAD)
	{
		return CELL_SPURS_TASK_ERROR_INVAL;
	}

	*wid = taskset->m.wid;
	return CELL_OK;
#endif
}

s64 cellSpursShutdownTaskset(vm::ptr<CellSpursTaskset> taskset)
{
	cellSpurs->Warning("cellSpursShutdownTaskset(taskset_addr=0x%x)", taskset.addr());

#ifdef PRX_DEBUG
	return GetCurrentPPUThread().FastCall2(libsre + 0x14868, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

u32 _cellSpursGetSdkVersion()
{
	static s32 sdk_version = -2;

	if (sdk_version == -2)
	{
		vm::var<be_t<s32>> version;
		sys_process_get_sdk_version(sys_process_getpid(), vm::ptr<s32>::make(version.addr()));
		sdk_version = version.value();
	}

	return sdk_version;
}

s64 spursCreateTask(vm::ptr<CellSpursTaskset> taskset, vm::ptr<u32> task_id, vm::ptr<u32> elf_addr, vm::ptr<u32> context_addr, u32 context_size, vm::ptr<CellSpursTaskLsPattern> ls_pattern, vm::ptr<CellSpursTaskArgument> arg)
{
	if (!taskset || !elf_addr)
	{
		return CELL_SPURS_TASK_ERROR_NULL_POINTER;
	}

	if (elf_addr.addr() % 16)
	{
		return CELL_SPURS_TASK_ERROR_ALIGN;
	}

	auto sdk_version = _cellSpursGetSdkVersion();
	if (sdk_version < 0x27FFFF)
	{
		if (context_addr.addr() % 16)
		{
			return CELL_SPURS_TASK_ERROR_ALIGN;
		}
	}
	else
	{
		if (context_addr.addr() % 128)
		{
			return CELL_SPURS_TASK_ERROR_ALIGN;
		}
	}

	u32 alloc_ls_blocks = 0;
	if (context_addr.addr() != 0)
	{
		if (context_size < CELL_SPURS_TASK_EXECUTION_CONTEXT_SIZE)
		{
			return CELL_SPURS_TASK_ERROR_INVAL;
		}

		alloc_ls_blocks = context_size > 0x3D400 ? 0x7A : ((context_size - 0x400) >> 11);
		if (ls_pattern.addr() != 0)
		{
			u32 ls_blocks = 0;
			for (u32 i = 0; i < 2; i++)
			{
				for (u32 j = 0; j < 64; j++)
				{
					if (ls_pattern->u64[0] & ((u64)1 << j))
					{
						ls_blocks++;
					}
				}
			}

			if (ls_blocks > alloc_ls_blocks)
			{
				return CELL_SPURS_TASK_ERROR_INVAL;
			}

			if (ls_pattern->u32[0] & 0xFC000000)
			{
				// Prevent save/restore to SPURS management area
				return CELL_SPURS_TASK_ERROR_INVAL;
			}
		}
	}
	else
	{
		alloc_ls_blocks = 0;
	}

	// TODO: Verify the ELF header is proper and all its load segments are at address >= 0x3000

	u32 tmp_task_id;
	for (tmp_task_id = 0; tmp_task_id < CELL_SPURS_MAX_TASK; tmp_task_id++)
	{
		u32 l = tmp_task_id >> 5;
		u32 b = tmp_task_id & 0x1F;
		if ((taskset->m.enabled_set[l] & (0x80000000 >> b)) == 0)
		{
			taskset->m.enabled_set[l] |= 0x80000000 >> b;
			break;
		}
	}

	if (tmp_task_id >= CELL_SPURS_MAX_TASK)
	{
		CELL_SPURS_TASK_ERROR_AGAIN;
	}

	taskset->m.task_info[tmp_task_id].elf_addr.set(elf_addr.addr());
	taskset->m.task_info[tmp_task_id].context_save_storage.set((context_addr.addr() & 0xFFFFFFF8) | alloc_ls_blocks);
	for (u32 i = 0; i < 2; i++)
	{
		taskset->m.task_info[tmp_task_id].args.u64[i] = arg != 0 ? arg->u64[i] : 0;
		taskset->m.task_info[tmp_task_id].ls_pattern.u64[i] = ls_pattern != 0 ? ls_pattern->u64[i] : 0;
	}

	*task_id = tmp_task_id;
	return CELL_OK;
}

s64 cellSpursCreateTask(vm::ptr<CellSpursTaskset> taskset, vm::ptr<u32> taskID, u32 elf_addr, u32 context_addr, u32 context_size, vm::ptr<CellSpursTaskLsPattern> lsPattern,
	vm::ptr<CellSpursTaskArgument> argument)
{
	cellSpurs->Warning("cellSpursCreateTask(taskset_addr=0x%x, taskID_addr=0x%x, elf_addr_addr=0x%x, context_addr_addr=0x%x, context_size=%d, lsPattern_addr=0x%x, argument_addr=0x%x)",
		taskset.addr(), taskID.addr(), elf_addr, context_addr, context_size, lsPattern.addr(), argument.addr());

#ifdef PRX_DEBUG
	return GetCurrentPPUThread().FastCall2(libsre + 0x12414, libsre_rtoc);
#else
	return spursCreateTask(taskset, taskID, vm::ptr<u32>::make(elf_addr), vm::ptr<u32>::make(context_addr), context_size, lsPattern, argument);
#endif
}

s64 _cellSpursSendSignal(vm::ptr<CellSpursTaskset> taskset, u32 taskID)
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("_cellSpursSendSignal(taskset_addr=0x%x, taskID=%d)", taskset.addr(), taskID);
	return GetCurrentPPUThread().FastCall2(libsre + 0x124CC, libsre_rtoc);
#else
	if (!taskset)
	{
		return CELL_SPURS_TASK_ERROR_NULL_POINTER;
	}

	if (taskset.addr() % CellSpursTaskset::align)
	{
		return CELL_SPURS_TASK_ERROR_ALIGN;
	}

	if (taskID >= CELL_SPURS_MAX_TASK || taskset->m.wid >= CELL_SPURS_MAX_WORKLOAD2)
	{
		return CELL_SPURS_TASK_ERROR_INVAL;
	}

	auto word      = taskID >> 5;
	auto mask      = 0x80000000u >> (taskID & 0x1F);
	auto disabled  = taskset->m.enabled_set[word] & mask ? false : true;
	auto running   = taskset->m.running_set[word];
	auto ready     = taskset->m.ready_set[word];
	auto ready2    = taskset->m.ready2_set[word];
	auto waiting   = taskset->m.waiting_set[word];
	auto signalled = taskset->m.signal_received_set[word];
	auto enabled   = taskset->m.enabled_set[word];
	auto invalid   = (ready & ready2) || (running & waiting) || ((running | ready | ready2 | waiting | signalled) & ~enabled) || disabled;

	if (invalid)
	{
		return CELL_SPURS_TASK_ERROR_SRCH;
	}

	auto shouldSignal = waiting & ~signalled & mask ? true : false;
	taskset->m.signal_received_set[word] |= mask;
	if (shouldSignal)
	{
		cellSpursSendWorkloadSignal(vm::ptr<CellSpurs>::make((u32)taskset->m.spurs.addr()), taskset->m.wid);
		auto rc = cellSpursWakeUp(GetCurrentPPUThread(), vm::ptr<CellSpurs>::make((u32)taskset->m.spurs.addr()));
		if (rc == CELL_SPURS_POLICY_MODULE_ERROR_STAT)
		{
			return CELL_SPURS_TASK_ERROR_STAT;
		}

		if (rc != CELL_OK)
		{
			assert(0);
		}
	}

	return CELL_OK;
#endif
}

s64 cellSpursCreateTaskWithAttribute()
{
	cellSpurs->Warning("%s()", __FUNCTION__);

#ifdef PRX_DEBUG
	return GetCurrentPPUThread().FastCall2(libsre + 0x12204, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursTasksetAttributeSetName(vm::ptr<CellSpursTasksetAttribute> attr, vm::ptr<const char> name)
{
	cellSpurs->Warning("%s(attr=0x%x, name=0x%x)", __FUNCTION__, attr.addr(), name.addr());

#ifdef PRX_DEBUG
	return GetCurrentPPUThread().FastCall2(libsre + 0x14210, libsre_rtoc);
#else
	if (!attr || !name)
	{
		return CELL_SPURS_TASK_ERROR_NULL_POINTER;
	}

	if (attr.addr() % CellSpursTasksetAttribute::align)
	{
		return CELL_SPURS_TASK_ERROR_ALIGN;
	}

	attr->m.name = name;
	return CELL_OK;
#endif
}

s64 cellSpursTasksetAttributeSetTasksetSize(vm::ptr<CellSpursTasksetAttribute> attr, u32 size)
{
	cellSpurs->Warning("%s(attr=0x%x, size=0x%x)", __FUNCTION__, attr.addr(), size);

#ifdef PRX_DEBUG
	return GetCurrentPPUThread().FastCall2(libsre + 0x14254, libsre_rtoc);
#else
	if (!attr)
	{
		return CELL_SPURS_TASK_ERROR_NULL_POINTER;
	}

	if (attr.addr() % CellSpursTasksetAttribute::align)
	{
		return CELL_SPURS_TASK_ERROR_ALIGN;
	}

	if (size != CellSpursTaskset::size && size != CellSpursTaskset2::size)
	{
		return CELL_SPURS_TASK_ERROR_INVAL;
	}

	attr->m.taskset_size = size;
	return CELL_OK;
#endif
}

s64 cellSpursTasksetAttributeEnableClearLS(vm::ptr<CellSpursTasksetAttribute> attr, s32 enable)
{
	cellSpurs->Warning("%s(attr=0x%x, enable=%d)", __FUNCTION__, attr.addr(), enable);

#ifdef PRX_DEBUG
	return GetCurrentPPUThread().FastCall2(libsre + 0x142AC, libsre_rtoc);
#else
	if (!attr)
	{
		return CELL_SPURS_TASK_ERROR_NULL_POINTER;
	}

	if (attr.addr() % CellSpursTasksetAttribute::align)
	{
		return CELL_SPURS_TASK_ERROR_ALIGN;
	}

	attr->m.enable_clear_ls = enable ? 1 : 0;
	return CELL_OK;
#endif
}

s64 _cellSpursTasksetAttribute2Initialize(vm::ptr<CellSpursTasksetAttribute2> attribute, u32 revision)
{
	cellSpurs->Warning("_cellSpursTasksetAttribute2Initialize(attribute_addr=0x%x, revision=%d)", attribute.addr(), revision);
	
#ifdef PRX_DEBUG
	return GetCurrentPPUThread().FastCall2(libsre + 0x1474C, libsre_rtoc);
#else
	memset(attribute.get_ptr(), 0, CellSpursTasksetAttribute2::size);
	attribute->m.revision = revision;
	attribute->m.name.set(0);
	attribute->m.args = 0;

	for (s32 i = 0; i < 8; i++)
	{
		attribute->m.priority[i] = 1;
	}

	attribute->m.max_contention = 8;
	attribute->m.enable_clear_ls = 0;
	attribute->m.task_name_buffer.set(0);
	return CELL_OK;
#endif
}

s64 cellSpursTaskExitCodeGet()
{
	cellSpurs->Warning("%s()", __FUNCTION__);

#ifdef PRX_DEBUG
	return GetCurrentPPUThread().FastCall2(libsre + 0x1397C, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursTaskExitCodeInitialize()
{
	cellSpurs->Warning("%s()", __FUNCTION__);

#ifdef PRX_DEBUG
	return GetCurrentPPUThread().FastCall2(libsre + 0x1352C, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursTaskExitCodeTryGet()
{
	cellSpurs->Warning("%s()", __FUNCTION__);

#ifdef PRX_DEBUG
	return GetCurrentPPUThread().FastCall2(libsre + 0x13974, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursTaskGetLoadableSegmentPattern()
{
	cellSpurs->Warning("%s()", __FUNCTION__);

#ifdef PRX_DEBUG
	return GetCurrentPPUThread().FastCall2(libsre + 0x13ED4, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursTaskGetReadOnlyAreaPattern()
{
	cellSpurs->Warning("%s()", __FUNCTION__);

#ifdef PRX_DEBUG
	return GetCurrentPPUThread().FastCall2(libsre + 0x13CFC, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursTaskGenerateLsPattern()
{
	cellSpurs->Warning("%s()", __FUNCTION__);

#ifdef PRX_DEBUG
	return GetCurrentPPUThread().FastCall2(libsre + 0x13B78, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 _cellSpursTaskAttributeInitialize()
{
	cellSpurs->Warning("%s()", __FUNCTION__);

#ifdef PRX_DEBUG
	return GetCurrentPPUThread().FastCall2(libsre + 0x10C30, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursTaskAttributeSetExitCodeContainer()
{
	cellSpurs->Warning("%s()", __FUNCTION__);

#ifdef PRX_DEBUG
	return GetCurrentPPUThread().FastCall2(libsre + 0x10A98, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 _cellSpursTaskAttribute2Initialize(vm::ptr<CellSpursTaskAttribute2> attribute, u32 revision)
{
	cellSpurs->Warning("_cellSpursTaskAttribute2Initialize(attribute_addr=0x%x, revision=%d)", attribute.addr(), revision);

#ifdef PRX_DEBUG
	return GetCurrentPPUThread().FastCall2(libsre + 0x10B00, libsre_rtoc);
#else
	attribute->revision = revision;
	attribute->sizeContext = 0;
	attribute->eaContext = 0;
	
	for (s32 c = 0; c < 4; c++)
	{
		attribute->lsPattern.u32[c] = 0;
	}

	for (s32 i = 0; i < 2; i++)
	{
		attribute->lsPattern.u64[i] = 0;
	}

	attribute->name_addr = 0;

	return CELL_OK;
#endif
}

s64 cellSpursTaskGetContextSaveAreaSize()
{
	cellSpurs->Warning("%s()", __FUNCTION__);

#ifdef PRX_DEBUG
	return GetCurrentPPUThread().FastCall2(libsre + 0x1409C, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursCreateTaskset2(vm::ptr<CellSpurs> spurs, vm::ptr<CellSpursTaskset2> taskset, vm::ptr<CellSpursTasksetAttribute2> attr)
{
	cellSpurs->Warning("%s(spurs=0x%x, taskset=0x%x, attr=0x%x)", __FUNCTION__, spurs.addr(), taskset.addr(), attr.addr());

#ifdef PRX_DEBUG
	return GetCurrentPPUThread().FastCall2(libsre + 0x15108, libsre_rtoc);
#else
	vm::ptr<CellSpursTasksetAttribute2> tmp_attr;

	if (!attr)
	{
		attr.set(tmp_attr.addr());
		_cellSpursTasksetAttribute2Initialize(attr, 0);
	}

	auto rc = spursCreateTaskset(spurs, vm::ptr<CellSpursTaskset>::make(taskset.addr()), attr->m.args,
		vm::ptr<const u8[8]>::make(attr.addr() + offsetof(CellSpursTasksetAttribute, m.priority)),
		attr->m.max_contention, vm::ptr<const char>::make(attr->m.name.addr()), CellSpursTaskset2::size, (u8)attr->m.enable_clear_ls);
	if (rc != CELL_OK)
	{
		return rc;
	}

	if (attr->m.task_name_buffer.addr() % CellSpursTaskNameBuffer::align)
	{
		return CELL_SPURS_TASK_ERROR_ALIGN;
	}

	// TODO: Implement rest of the function
	return CELL_OK;
#endif
}

s64 cellSpursCreateTask2()
{
	cellSpurs->Warning("%s()", __FUNCTION__);

#ifdef PRX_DEBUG
	return GetCurrentPPUThread().FastCall2(libsre + 0x11E54, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursJoinTask2()
{
	cellSpurs->Warning("%s()", __FUNCTION__);

#ifdef PRX_DEBUG
	return GetCurrentPPUThread().FastCall2(libsre + 0x11378, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursTryJoinTask2()
{
	cellSpurs->Warning("%s()", __FUNCTION__);

#ifdef PRX_DEBUG
	return GetCurrentPPUThread().FastCall2(libsre + 0x11748, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursDestroyTaskset2()
{
	cellSpurs->Warning("%s()", __FUNCTION__);

#ifdef PRX_DEBUG
	return GetCurrentPPUThread().FastCall2(libsre + 0x14EE8, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursCreateTask2WithBinInfo()
{
	cellSpurs->Warning("%s()", __FUNCTION__);

#ifdef PRX_DEBUG
	return GetCurrentPPUThread().FastCall2(libsre + 0x120E0, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursTasksetSetExceptionEventHandler(vm::ptr<CellSpursTaskset> taskset, vm::ptr<u64> handler, vm::ptr<u64> arg)
{
	cellSpurs->Warning("%s(taskset=0x5x, handler=0x%x, arg=0x%x)", __FUNCTION__, taskset.addr(), handler.addr(), arg.addr());

#ifdef PRX_DEBUG
	return GetCurrentPPUThread().FastCall2(libsre + 0x13124, libsre_rtoc);
#else
	if (!taskset || !handler)
	{
		return CELL_SPURS_TASK_ERROR_NULL_POINTER;
	}

	if (taskset.addr() % CellSpursTaskset::align)
	{
		return CELL_SPURS_TASK_ERROR_ALIGN;
	}

	if (taskset->m.wid >= CELL_SPURS_MAX_WORKLOAD)
	{
		return CELL_SPURS_TASK_ERROR_INVAL;
	}

	if (taskset->m.exception_handler != 0)
	{
		return CELL_SPURS_TASK_ERROR_BUSY;
	}

	taskset->m.exception_handler = handler;
	taskset->m.exception_handler_arg = arg;
	return CELL_OK;
#endif
}

s64 cellSpursTasksetUnsetExceptionEventHandler(vm::ptr<CellSpursTaskset> taskset)
{
	cellSpurs->Warning("%s(taskset=0x%x)", __FUNCTION__, taskset.addr());

#ifdef PRX_DEBUG
	return GetCurrentPPUThread().FastCall2(libsre + 0x13194, libsre_rtoc);
#else
	if (!taskset)
	{
		return CELL_SPURS_TASK_ERROR_NULL_POINTER;
	}

	if (taskset.addr() % CellSpursTaskset::align)
	{
		return CELL_SPURS_TASK_ERROR_ALIGN;
	}

	if (taskset->m.wid >= CELL_SPURS_MAX_WORKLOAD)
	{
		return CELL_SPURS_TASK_ERROR_INVAL;
	}

	taskset->m.exception_handler.set(0);
	taskset->m.exception_handler_arg.set(0);
	return CELL_OK;
#endif
}

s64 cellSpursLookUpTasksetAddress(vm::ptr<CellSpurs> spurs, vm::ptr<CellSpursTaskset> taskset, u32 id)
{
	cellSpurs->Warning("%s(spurs=0x%x, taskset=0x%x, id=0x%x)", __FUNCTION__, spurs.addr(), taskset.addr(), id);

#ifdef PRX_DEBUG
	return GetCurrentPPUThread().FastCall2(libsre + 0x133AC, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursTasksetGetSpursAddress(vm::ptr<const CellSpursTaskset> taskset, vm::ptr<u32> spurs)
{
	cellSpurs->Warning("%s(taskset=0x%x, spurs=0x%x)", __FUNCTION__, taskset.addr(), spurs.addr());

#ifdef PRX_DEBUG
	return GetCurrentPPUThread().FastCall2(libsre + 0x14408, libsre_rtoc);
#else
	if (!taskset || !spurs)
	{
		return CELL_SPURS_TASK_ERROR_NULL_POINTER;
	}

	if (taskset.addr() % CellSpursTaskset::align)
	{
		return CELL_SPURS_TASK_ERROR_ALIGN;
	}

	if (taskset->m.wid >= CELL_SPURS_MAX_WORKLOAD)
	{
		return CELL_SPURS_TASK_ERROR_INVAL;
	}

	*spurs = (u32)taskset->m.spurs.addr();
	return CELL_OK;
#endif
}

s64 cellSpursGetTasksetInfo()
{
	cellSpurs->Warning("%s()", __FUNCTION__);

#ifdef PRX_DEBUG
	return GetCurrentPPUThread().FastCall2(libsre + 0x1445C, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 _cellSpursTasksetAttributeInitialize(vm::ptr<CellSpursTasksetAttribute> attribute, u32 revision, u32 sdk_version, u64 args, vm::ptr<const u8> priority, u32 max_contention)
{
	cellSpurs->Warning("%s(attribute=0x%x, revision=%u, skd_version=%u, args=0x%llx, priority=0x%x, max_contention=%u)",
		__FUNCTION__, attribute.addr(), revision, sdk_version, args, priority.addr(), max_contention);

#ifdef PRX_DEBUG
	return GetCurrentPPUThread().FastCall2(libsre + 0x142FC, libsre_rtoc);
#else
	if (!attribute)
	{
		return CELL_SPURS_TASK_ERROR_NULL_POINTER;
	}

	if (attribute.addr() % CellSpursTasksetAttribute::align)
	{
		return CELL_SPURS_TASK_ERROR_ALIGN;
	}

	for (u32 i = 0; i < 8; i++)
	{
		if (priority[i] > 0xF)
		{
			return CELL_SPURS_TASK_ERROR_INVAL;
		}
	}

	memset(attribute.get_ptr(), 0, CellSpursTasksetAttribute::size);
	attribute->m.revision = revision;
	attribute->m.sdk_version = sdk_version;
	attribute->m.args = args;
	memcpy(attribute->m.priority, priority.get_ptr(), 8);
	attribute->m.taskset_size = CellSpursTaskset::size;
	attribute->m.max_contention = max_contention;
	return CELL_OK;
#endif
}

s64 cellSpursJobGuardInitialize()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x1807C, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursJobChainAttributeSetName()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x1861C, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursShutdownJobChain()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x18D2C, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursJobChainAttributeSetHaltOnError()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x18660, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursJobChainAttributeSetJobTypeMemoryCheck()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x186A4, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursJobGuardNotify()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x17FA4, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursJobGuardReset()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x17F60, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursRunJobChain()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x18F94, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursJobChainGetError()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x190AC, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursGetJobPipelineInfo()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x1A954, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursJobSetMaxGrab()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x1AC88, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursJobHeaderSetJobbin2Param()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x1AD58, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursAddUrgentCommand()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x18160, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursAddUrgentCall()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x1823C, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursBarrierInitialize()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x17CD8, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursBarrierGetTasksetAddress()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x17DB0, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 _cellSpursSemaphoreInitialize()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x17DF8, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursSemaphoreGetTasksetAddress()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x17F18, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

bool spursIsLibProfLoaded()
{
	return false;
}

void spursTraceStatusUpdate(vm::ptr<CellSpurs> spurs)
{
	LV2_LOCK(0);

	if (spurs->m.xCC != 0)
	{
		spurs->m.xCD                  = 1;
		spurs->m.sysSrvMsgUpdateTrace = (1 << spurs->m.nSpus) - 1;
		spurs->m.sysSrvMessage.write_relaxed(0xFF);
		sys_semaphore_wait((u32)spurs->m.semPrv, 0);
	}
}

s64 spursTraceInitialize(vm::ptr<CellSpurs> spurs, vm::ptr<CellSpursTraceInfo> buffer, u32 size, u32 mode, u32 updateStatus)
{
	if (!spurs || !buffer)
	{
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;
	}

	if (spurs.addr() % CellSpurs::align || buffer.addr() % CellSpursTraceInfo::align)
	{
		return CELL_SPURS_CORE_ERROR_ALIGN;
	}

	if (size < CellSpursTraceInfo::size || mode & ~(CELL_SPURS_TRACE_MODE_FLAG_MASK))
	{
		return CELL_SPURS_CORE_ERROR_INVAL;
	}

	if (spurs->m.traceBuffer != 0)
	{
		return CELL_SPURS_CORE_ERROR_STAT;
	}

	spurs->m.traceDataSize = size - CellSpursTraceInfo::size;
	for (u32 i = 0; i < 8; i++)
	{
		buffer->spu_thread[i] = spurs->m.spus[i];
		buffer->count[i]      = 0;
	}

	buffer->spu_thread_grp = spurs->m.spuTG;
	buffer->nspu           = spurs->m.nSpus;
	spurs->m.traceBuffer.set(buffer.addr() | (mode & CELL_SPURS_TRACE_MODE_FLAG_WRAP_BUFFER ? 1 : 0));
	spurs->m.traceMode     = mode;

	u32 spuTraceDataCount = (u32)((spurs->m.traceDataSize / CellSpursTracePacket::size) / spurs->m.nSpus);
	for (u32 i = 0, j = 8; i < 6; i++)
	{
		spurs->m.traceStartIndex[i] = j;
		j += spuTraceDataCount;
	}

	spurs->m.sysSrvTraceControl = 0;
	if (updateStatus)
	{
		spursTraceStatusUpdate(spurs);
	}

	return CELL_OK;
}

s64 cellSpursTraceInitialize(vm::ptr<CellSpurs> spurs, vm::ptr<CellSpursTraceInfo> buffer, u32 size, u32 mode)
{
	if (spursIsLibProfLoaded())
	{
		return CELL_SPURS_CORE_ERROR_STAT;
	}

	return spursTraceInitialize(spurs, buffer, size, mode, 1);
}

s64 spursTraceStart(vm::ptr<CellSpurs> spurs, u32 updateStatus)
{
	if (!spurs)
	{
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;
	}

	if (spurs.addr() % CellSpurs::align)
	{
		return CELL_SPURS_CORE_ERROR_ALIGN;
	}

	if (!spurs->m.traceBuffer)
	{
		return CELL_SPURS_CORE_ERROR_STAT;
	}

	spurs->m.sysSrvTraceControl = 1;
	if (updateStatus)
	{
		spursTraceStatusUpdate(spurs);
	}

	return CELL_OK;
}

s64 cellSpursTraceStart(vm::ptr<CellSpurs> spurs)
{
	if (!spurs)
	{
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;
	}

	if (spurs.addr() % CellSpurs::align)
	{
		return CELL_SPURS_CORE_ERROR_ALIGN;
	}

	return spursTraceStart(spurs, spurs->m.traceMode & CELL_SPURS_TRACE_MODE_FLAG_SYNCHRONOUS_START_STOP);
}

s64 spursTraceStop(vm::ptr<CellSpurs> spurs, u32 updateStatus)
{
	if (!spurs)
	{
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;
	}

	if (spurs.addr() % CellSpurs::align)
	{
		return CELL_SPURS_CORE_ERROR_ALIGN;
	}

	if (!spurs->m.traceBuffer)
	{
		return CELL_SPURS_CORE_ERROR_STAT;
	}

	spurs->m.sysSrvTraceControl = 2;
	if (updateStatus)
	{
		spursTraceStatusUpdate(spurs);
	}

	return CELL_OK;
}

s64 cellSpursTraceStop(vm::ptr<CellSpurs> spurs)
{
	if (!spurs)
	{
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;
	}

	if (spurs.addr() % CellSpurs::align)
	{
		return CELL_SPURS_CORE_ERROR_ALIGN;
	}

	return spursTraceStop(spurs, spurs->m.traceMode & CELL_SPURS_TRACE_MODE_FLAG_SYNCHRONOUS_START_STOP);
}

s64 cellSpursTraceFinalize(vm::ptr<CellSpurs> spurs)
{
	if (!spurs)
	{
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;
	}

	if (spurs.addr() % CellSpurs::align)
	{
		return CELL_SPURS_CORE_ERROR_ALIGN;
	}

	if (!spurs->m.traceBuffer)
	{
		return CELL_SPURS_CORE_ERROR_STAT;
	}

	spurs->m.sysSrvTraceControl = 0;
	spurs->m.traceMode          = 0;
	spurs->m.traceBuffer.set(0);
	spursTraceStatusUpdate(spurs);
	return CELL_OK;
}

void cellSpurs_init(Module *pxThis)
{
	cellSpurs = pxThis;

	// Core 
	REG_FUNC(cellSpurs, cellSpursInitialize);
	REG_FUNC(cellSpurs, cellSpursInitializeWithAttribute);
	REG_FUNC(cellSpurs, cellSpursInitializeWithAttribute2);
	REG_FUNC(cellSpurs, cellSpursFinalize);
	REG_FUNC(cellSpurs, _cellSpursAttributeInitialize);
	REG_FUNC(cellSpurs, cellSpursAttributeSetMemoryContainerForSpuThread);
	REG_FUNC(cellSpurs, cellSpursAttributeSetNamePrefix);
	REG_FUNC(cellSpurs, cellSpursAttributeEnableSpuPrintfIfAvailable);
	REG_FUNC(cellSpurs, cellSpursAttributeSetSpuThreadGroupType);
	REG_FUNC(cellSpurs, cellSpursAttributeEnableSystemWorkload);
	REG_FUNC(cellSpurs, cellSpursGetSpuThreadGroupId);
	REG_FUNC(cellSpurs, cellSpursGetNumSpuThread);
	REG_FUNC(cellSpurs, cellSpursGetSpuThreadId);
	REG_FUNC(cellSpurs, cellSpursGetInfo);
	REG_FUNC(cellSpurs, cellSpursSetMaxContention);
	REG_FUNC(cellSpurs, cellSpursSetPriorities);
	REG_FUNC(cellSpurs, cellSpursSetPreemptionVictimHints);
	REG_FUNC(cellSpurs, cellSpursAttachLv2EventQueue);
	REG_FUNC(cellSpurs, cellSpursDetachLv2EventQueue);
	REG_FUNC(cellSpurs, cellSpursEnableExceptionEventHandler);
	REG_FUNC(cellSpurs, cellSpursSetGlobalExceptionEventHandler);
	REG_FUNC(cellSpurs, cellSpursUnsetGlobalExceptionEventHandler);
	REG_FUNC(cellSpurs, cellSpursSetExceptionEventHandler);
	REG_FUNC(cellSpurs, cellSpursUnsetExceptionEventHandler);

	// Event flag
	REG_FUNC(cellSpurs, _cellSpursEventFlagInitialize);
	REG_FUNC(cellSpurs, cellSpursEventFlagAttachLv2EventQueue);
	REG_FUNC(cellSpurs, cellSpursEventFlagDetachLv2EventQueue);
	REG_FUNC(cellSpurs, cellSpursEventFlagWait);
	REG_FUNC(cellSpurs, cellSpursEventFlagClear);
	REG_FUNC(cellSpurs, cellSpursEventFlagSet);
	REG_FUNC(cellSpurs, cellSpursEventFlagTryWait);
	REG_FUNC(cellSpurs, cellSpursEventFlagGetDirection);
	REG_FUNC(cellSpurs, cellSpursEventFlagGetClearMode);
	REG_FUNC(cellSpurs, cellSpursEventFlagGetTasksetAddress);

	// Taskset
	REG_FUNC(cellSpurs, cellSpursCreateTaskset);
	REG_FUNC(cellSpurs, cellSpursCreateTasksetWithAttribute);
	REG_FUNC(cellSpurs, _cellSpursTasksetAttributeInitialize);
	REG_FUNC(cellSpurs, _cellSpursTasksetAttribute2Initialize);
	REG_FUNC(cellSpurs, cellSpursTasksetAttributeSetName);
	REG_FUNC(cellSpurs, cellSpursTasksetAttributeSetTasksetSize);
	REG_FUNC(cellSpurs, cellSpursTasksetAttributeEnableClearLS);
	REG_FUNC(cellSpurs, cellSpursJoinTaskset);
	REG_FUNC(cellSpurs, cellSpursGetTasksetId);
	REG_FUNC(cellSpurs, cellSpursShutdownTaskset);
	REG_FUNC(cellSpurs, cellSpursCreateTask);
	REG_FUNC(cellSpurs, cellSpursCreateTaskWithAttribute);
	REG_FUNC(cellSpurs, _cellSpursTaskAttributeInitialize);
	REG_FUNC(cellSpurs, _cellSpursTaskAttribute2Initialize);
	REG_FUNC(cellSpurs, cellSpursTaskAttributeSetExitCodeContainer);
	REG_FUNC(cellSpurs, cellSpursTaskExitCodeGet);
	REG_FUNC(cellSpurs, cellSpursTaskExitCodeInitialize);
	REG_FUNC(cellSpurs, cellSpursTaskExitCodeTryGet);
	REG_FUNC(cellSpurs, cellSpursTaskGetLoadableSegmentPattern);
	REG_FUNC(cellSpurs, cellSpursTaskGetReadOnlyAreaPattern);
	REG_FUNC(cellSpurs, cellSpursTaskGenerateLsPattern);
	REG_FUNC(cellSpurs, cellSpursTaskGetContextSaveAreaSize);
	REG_FUNC(cellSpurs, _cellSpursSendSignal);
	REG_FUNC(cellSpurs, cellSpursCreateTaskset2);
	REG_FUNC(cellSpurs, cellSpursCreateTask2);
	REG_FUNC(cellSpurs, cellSpursJoinTask2);
	REG_FUNC(cellSpurs, cellSpursTryJoinTask2);
	REG_FUNC(cellSpurs, cellSpursDestroyTaskset2);
	REG_FUNC(cellSpurs, cellSpursCreateTask2WithBinInfo);
	REG_FUNC(cellSpurs, cellSpursLookUpTasksetAddress);
	REG_FUNC(cellSpurs, cellSpursTasksetGetSpursAddress);
	REG_FUNC(cellSpurs, cellSpursGetTasksetInfo);
	REG_FUNC(cellSpurs, cellSpursTasksetSetExceptionEventHandler);
	REG_FUNC(cellSpurs, cellSpursTasksetUnsetExceptionEventHandler);

	// Job Chain
	REG_FUNC(cellSpurs, cellSpursCreateJobChain);
	REG_FUNC(cellSpurs, cellSpursCreateJobChainWithAttribute);
	REG_FUNC(cellSpurs, cellSpursShutdownJobChain);
	REG_FUNC(cellSpurs, cellSpursJoinJobChain);
	REG_FUNC(cellSpurs, cellSpursKickJobChain);
	REG_FUNC(cellSpurs, cellSpursRunJobChain);
	REG_FUNC(cellSpurs, cellSpursJobChainGetError);
	REG_FUNC(cellSpurs, _cellSpursJobChainAttributeInitialize);
	REG_FUNC(cellSpurs, cellSpursJobChainAttributeSetName);
	REG_FUNC(cellSpurs, cellSpursJobChainAttributeSetHaltOnError);
	REG_FUNC(cellSpurs, cellSpursJobChainAttributeSetJobTypeMemoryCheck);
	REG_FUNC(cellSpurs, cellSpursGetJobChainId);
	REG_FUNC(cellSpurs, cellSpursJobChainSetExceptionEventHandler);
	REG_FUNC(cellSpurs, cellSpursJobChainUnsetExceptionEventHandler);
	REG_FUNC(cellSpurs, cellSpursGetJobChainInfo);
	REG_FUNC(cellSpurs, cellSpursJobChainGetSpursAddress);

	// Job Guard
	REG_FUNC(cellSpurs, cellSpursJobGuardInitialize);
	REG_FUNC(cellSpurs, cellSpursJobGuardNotify);
	REG_FUNC(cellSpurs, cellSpursJobGuardReset);
	
	// LFQueue
	REG_FUNC(cellSpurs, _cellSpursLFQueueInitialize);
	REG_FUNC(cellSpurs, _cellSpursLFQueuePushBody);
	REG_FUNC(cellSpurs, cellSpursLFQueueAttachLv2EventQueue);
	REG_FUNC(cellSpurs, cellSpursLFQueueDetachLv2EventQueue);
	REG_FUNC(cellSpurs, _cellSpursLFQueuePopBody);
	REG_FUNC(cellSpurs, cellSpursLFQueueGetTasksetAddress);

	// Queue
	REG_FUNC(cellSpurs, _cellSpursQueueInitialize);
	REG_FUNC(cellSpurs, cellSpursQueuePopBody);
	REG_FUNC(cellSpurs, cellSpursQueuePushBody);
	REG_FUNC(cellSpurs, cellSpursQueueAttachLv2EventQueue);
	REG_FUNC(cellSpurs, cellSpursQueueDetachLv2EventQueue);
	REG_FUNC(cellSpurs, cellSpursQueueGetTasksetAddress);
	REG_FUNC(cellSpurs, cellSpursQueueClear);
	REG_FUNC(cellSpurs, cellSpursQueueDepth);
	REG_FUNC(cellSpurs, cellSpursQueueGetEntrySize);
	REG_FUNC(cellSpurs, cellSpursQueueSize);
	REG_FUNC(cellSpurs, cellSpursQueueGetDirection);

	// Workload
	REG_FUNC(cellSpurs, cellSpursWorkloadAttributeSetName);
	REG_FUNC(cellSpurs, cellSpursWorkloadAttributeSetShutdownCompletionEventHook);
	REG_FUNC(cellSpurs, cellSpursAddWorkloadWithAttribute);
	REG_FUNC(cellSpurs, cellSpursAddWorkload);
	REG_FUNC(cellSpurs, cellSpursShutdownWorkload);
	REG_FUNC(cellSpurs, cellSpursWaitForWorkloadShutdown);
	REG_FUNC(cellSpurs, cellSpursRemoveWorkload);
	REG_FUNC(cellSpurs, cellSpursReadyCountStore);
	REG_FUNC(cellSpurs, cellSpursGetWorkloadFlag);
	REG_FUNC(cellSpurs, _cellSpursWorkloadFlagReceiver);
	REG_FUNC(cellSpurs, _cellSpursWorkloadAttributeInitialize);
	REG_FUNC(cellSpurs, cellSpursSendWorkloadSignal);
	REG_FUNC(cellSpurs, cellSpursGetWorkloadData);
	REG_FUNC(cellSpurs, cellSpursReadyCountAdd);
	REG_FUNC(cellSpurs, cellSpursReadyCountCompareAndSwap);
	REG_FUNC(cellSpurs, cellSpursReadyCountSwap);
	REG_FUNC(cellSpurs, cellSpursRequestIdleSpu);
	REG_FUNC(cellSpurs, cellSpursGetWorkloadInfo);
	REG_FUNC(cellSpurs, cellSpursGetSpuGuid);
	REG_FUNC(cellSpurs, _cellSpursWorkloadFlagReceiver2);
	REG_FUNC(cellSpurs, cellSpursGetJobPipelineInfo);
	REG_FUNC(cellSpurs, cellSpursJobSetMaxGrab);
	REG_FUNC(cellSpurs, cellSpursJobHeaderSetJobbin2Param);

	REG_FUNC(cellSpurs, cellSpursWakeUp);
	REG_FUNC(cellSpurs, cellSpursAddUrgentCommand);
	REG_FUNC(cellSpurs, cellSpursAddUrgentCall);

	REG_FUNC(cellSpurs, cellSpursBarrierInitialize);
	REG_FUNC(cellSpurs, cellSpursBarrierGetTasksetAddress);

	REG_FUNC(cellSpurs, _cellSpursSemaphoreInitialize);
	REG_FUNC(cellSpurs, cellSpursSemaphoreGetTasksetAddress);

	// Trace
	REG_FUNC(cellSpurs, cellSpursTraceInitialize);
	REG_FUNC(cellSpurs, cellSpursTraceStart);
	REG_FUNC(cellSpurs, cellSpursTraceStop);
	REG_FUNC(cellSpurs, cellSpursTraceFinalize);
}