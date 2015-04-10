#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/SysCalls/Modules.h"
#include "Emu/SysCalls/CB_FUNC.h"

#include "Emu/CPU/CPUThreadManager.h"
#include "Emu/Cell/SPUThread.h"
#include "Emu/SysCalls/lv2/sleep_queue.h"
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

extern Module cellSpurs;

bool spursKernelEntry(SPUThread & spu);
s32 cellSpursLookUpTasksetAddress(vm::ptr<CellSpurs> spurs, vm::ptr<CellSpursTaskset> taskset, u32 id);
s32 _cellSpursSendSignal(vm::ptr<CellSpursTaskset> taskset, u32 taskID);

s32 spursCreateLv2EventQueue(vm::ptr<CellSpurs> spurs, u32& queue_id, vm::ptr<u8> port, s32 size, u64 name_u64)
{
	queue_id = event_queue_create(SYS_SYNC_FIFO, SYS_PPU_QUEUE, name_u64, 0, size);
	if (!queue_id)
	{
		return CELL_EAGAIN; // rough
	}

	if (s32 res = spursAttachLv2EventQueue(spurs, queue_id, port, 1, true))
	{
		assert(!"spursAttachLv2EventQueue() failed");
	}
	return CELL_OK;
}

s32 spursInit(
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
	spurs->m.wklInfoSysSrv.addr.set(be_t<u64>::make(SPURS_IMG_ADDR_SYS_SRV_WORKLOAD));
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

	spurs->m.spuImg.addr        = Memory.MainMem.AllocAlign(0x40000, 4096);
	spurs->m.spuImg.entry_point = isSecond ? CELL_SPURS_KERNEL2_ENTRY_ADDR : CELL_SPURS_KERNEL1_ENTRY_ADDR;

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
	spurs->m.spuTG = spu_thread_group_create(name + "CellSpursKernelGroup", nSpus, spuPriority, tgt, container);
	assert(spurs->m.spuTG.data());

	name += "CellSpursKernel0";
	for (s32 num = 0; num < nSpus; num++, name[name.size() - 1]++)
	{
		const u32 id = spu_thread_initialize(spurs->m.spuTG, num, vm::ptr<sys_spu_image>::make(spurs.addr() + offsetof(CellSpurs, m.spuImg)), name, SYS_SPU_THREAD_OPTION_DEC_SYNC_TB_ENABLE, (u64)num << 32, spurs.addr(), 0, 0);

		static_cast<SPUThread&>(*Emu.GetCPU().GetThread(id).get()).RegisterHleFunction(spurs->m.spuImg.entry_point, spursKernelEntry);

		spurs->m.spus[num] = id;
	}

	if (flags & SAF_SPU_PRINTF_ENABLED)
	{
		// spu_printf: attach group
		if (!spu_printf_agcb || spu_printf_agcb(spurs->m.spuTG) != CELL_OK)
		{
			// remove flag if failed
			spurs->m.flags &= ~SAF_SPU_PRINTF_ENABLED;
		}
	}

	lwmutex_create(spurs->m.mutex, false, SYS_SYNC_PRIORITY, *(u64*)"_spuPrv");
	lwcond_create(spurs->m.cond, spurs->m.mutex, *(u64*)"_spuPrv");

	spurs->m.flags1 = (flags & SAF_EXIT_IF_NO_WORK ? SF1_EXIT_IF_NO_WORK : 0) | (isSecond ? SF1_32_WORKLOADS : 0);
	spurs->m.wklFlagReceiver.write_relaxed(0xff);
	spurs->m.wklFlag.flag.write_relaxed(be_t<u32>::make(-1));
	spurs->_u8[0xD64] = 0;
	spurs->_u8[0xD65] = 0;
	spurs->_u8[0xD66] = 0;
	spurs->m.ppuPriority = ppuPriority;

	u32 queue;
	if (s32 res = spursCreateLv2EventQueue(spurs, queue, vm::ptr<u8>::make(spurs.addr() + 0xc9), 0x2a, *(u64*)"_spuPrv"))
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
		if (spurs->m.flags & SAF_UNKNOWN_FLAG_30)
		{
			return;
		}

		while (true)
		{
			if (Emu.IsStopped())
			{
				cellSpurs.Warning("SPURS Handler Thread 0 aborted");
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
	});

	spurs->m.ppu1 = ppu_thread_create(0, 0, ppuPriority, 0x8000, true, false, name + "SpursHdlr1", [spurs](PPUThread& CPU)
	{
		// TODO

	});

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

	if (flags & SAF_SYSTEM_WORKLOAD_ENABLED) // initialize system workload (disabled)
	{
		s32 res = CELL_OK;
		// TODO
		assert(res == CELL_OK);
	}
	else if (flags & SAF_EXIT_IF_NO_WORK) // wakeup
	{
		return spursWakeUp(GetCurrentPPUThread(), spurs);
	}

	return CELL_OK;
}

s32 cellSpursInitialize(vm::ptr<CellSpurs> spurs, s32 nSpus, s32 spuPriority, s32 ppuPriority, bool exitIfNoWork)
{
	cellSpurs.Warning("cellSpursInitialize(spurs_addr=0x%x, nSpus=%d, spuPriority=%d, ppuPriority=%d, exitIfNoWork=%d)",
		spurs.addr(), nSpus, spuPriority, ppuPriority, exitIfNoWork ? 1 : 0);

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

s32 cellSpursInitializeWithAttribute(vm::ptr<CellSpurs> spurs, vm::ptr<const CellSpursAttribute> attr)
{
	cellSpurs.Warning("cellSpursInitializeWithAttribute(spurs_addr=0x%x, attr_addr=0x%x)", spurs.addr(), attr.addr());

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

s32 cellSpursInitializeWithAttribute2(vm::ptr<CellSpurs> spurs, vm::ptr<const CellSpursAttribute> attr)
{
	cellSpurs.Warning("cellSpursInitializeWithAttribute2(spurs_addr=0x%x, attr_addr=0x%x)", spurs.addr(), attr.addr());

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

s32 _cellSpursAttributeInitialize(vm::ptr<CellSpursAttribute> attr, u32 revision, u32 sdkVersion, u32 nSpus, s32 spuPriority, s32 ppuPriority, bool exitIfNoWork)
{
	cellSpurs.Warning("_cellSpursAttributeInitialize(attr_addr=0x%x, revision=%d, sdkVersion=0x%x, nSpus=%d, spuPriority=%d, ppuPriority=%d, exitIfNoWork=%d)",
		attr.addr(), revision, sdkVersion, nSpus, spuPriority, ppuPriority, exitIfNoWork ? 1 : 0);

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

s32 cellSpursAttributeSetMemoryContainerForSpuThread(vm::ptr<CellSpursAttribute> attr, u32 container)
{
	cellSpurs.Warning("cellSpursAttributeSetMemoryContainerForSpuThread(attr_addr=0x%x, container=%d)", attr.addr(), container);

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

s32 cellSpursAttributeSetNamePrefix(vm::ptr<CellSpursAttribute> attr, vm::ptr<const char> prefix, u32 size)
{
	cellSpurs.Warning("cellSpursAttributeSetNamePrefix(attr_addr=0x%x, prefix_addr=0x%x, size=%d)", attr.addr(), prefix.addr(), size);

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

s32 cellSpursAttributeEnableSpuPrintfIfAvailable(vm::ptr<CellSpursAttribute> attr)
{
	cellSpurs.Warning("cellSpursAttributeEnableSpuPrintfIfAvailable(attr_addr=0x%x)", attr.addr());

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

s32 cellSpursAttributeSetSpuThreadGroupType(vm::ptr<CellSpursAttribute> attr, s32 type)
{
	cellSpurs.Warning("cellSpursAttributeSetSpuThreadGroupType(attr_addr=0x%x, type=%d)", attr.addr(), type);

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

s32 cellSpursAttributeEnableSystemWorkload(vm::ptr<CellSpursAttribute> attr, vm::ptr<const u8[8]> priority, u32 maxSpu, vm::ptr<const bool[8]> isPreemptible)
{
	cellSpurs.Warning("cellSpursAttributeEnableSystemWorkload(attr_addr=0x%x, priority_addr=0x%x, maxSpu=%d, isPreemptible_addr=0x%x)",
		attr.addr(), priority.addr(), maxSpu, isPreemptible.addr());

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

s32 cellSpursFinalize(vm::ptr<CellSpurs> spurs)
{
	cellSpurs.Todo("cellSpursFinalize(spurs_addr=0x%x)", spurs.addr());

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

	// TODO

	return CELL_OK;
}

s32 spursAttachLv2EventQueue(vm::ptr<CellSpurs> spurs, u32 queue, vm::ptr<u8> port, s32 isDynamic, bool wasCreated)
{
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

s32 cellSpursAttachLv2EventQueue(vm::ptr<CellSpurs> spurs, u32 queue, vm::ptr<u8> port, s32 isDynamic)
{
	cellSpurs.Warning("cellSpursAttachLv2EventQueue(spurs_addr=0x%x, queue=%d, port_addr=0x%x, isDynamic=%d)",
		spurs.addr(), queue, port.addr(), isDynamic);

	return spursAttachLv2EventQueue(spurs, queue, port, isDynamic, false);
}

s32 cellSpursDetachLv2EventQueue(vm::ptr<CellSpurs> spurs, u8 port)
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 cellSpursGetSpuGuid()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 cellSpursGetSpuThreadGroupId(vm::ptr<CellSpurs> spurs, vm::ptr<u32> group)
{
	cellSpurs.Warning("cellSpursGetSpuThreadGroupId(spurs_addr=0x%x, group_addr=0x%x)", spurs.addr(), group.addr());

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

s32 cellSpursGetNumSpuThread(vm::ptr<CellSpurs> spurs, vm::ptr<u32> nThreads)
{
	cellSpurs.Warning("cellSpursGetNumSpuThread(spurs_addr=0x%x, nThreads_addr=0x%x)", spurs.addr(), nThreads.addr());

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

s32 cellSpursGetSpuThreadId(vm::ptr<CellSpurs> spurs, vm::ptr<u32> thread, vm::ptr<u32> nThreads)
{
	cellSpurs.Warning("cellSpursGetSpuThreadId(spurs_addr=0x%x, thread_addr=0x%x, nThreads_addr=0x%x)", spurs.addr(), thread.addr(), nThreads.addr());

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

s32 cellSpursSetMaxContention(vm::ptr<CellSpurs> spurs, u32 workloadId, u32 maxContention)
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 cellSpursSetPriorities(vm::ptr<CellSpurs> spurs, u32 workloadId, vm::ptr<const u8> priorities)
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 cellSpursSetPreemptionVictimHints(vm::ptr<CellSpurs> spurs, vm::ptr<const bool> isPreemptible)
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 cellSpursEnableExceptionEventHandler(vm::ptr<CellSpurs> spurs, bool flag)
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 cellSpursSetGlobalExceptionEventHandler(vm::ptr<CellSpurs> spurs, u32 eaHandler_addr, u32 arg_addr)
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 cellSpursUnsetGlobalExceptionEventHandler(vm::ptr<CellSpurs> spurs)
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 cellSpursGetInfo(vm::ptr<CellSpurs> spurs, vm::ptr<CellSpursInfo> info)
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 spursWakeUp(PPUThread& CPU, vm::ptr<CellSpurs> spurs)
{
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
		if (s32 res = sys_lwcond_signal(CPU, spurs->get_lwcond()))
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

s32 cellSpursWakeUp(PPUThread& CPU, vm::ptr<CellSpurs> spurs)
{
	cellSpurs.Warning("%s(spurs_addr=0x%x)", __FUNCTION__, spurs.addr());

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

s32 cellSpursAddWorkload(
	vm::ptr<CellSpurs> spurs,
	vm::ptr<u32> wid,
	vm::ptr<const void> pm,
	u32 size,
	u64 data,
	vm::ptr<const u8[8]> priorityTable,
	u32 minContention,
	u32 maxContention)
{
	cellSpurs.Warning("%s(spurs_addr=0x%x, wid_addr=0x%x, pm_addr=0x%x, size=0x%x, data=0x%llx, priorityTable_addr=0x%x, minContention=0x%x, maxContention=0x%x)",
		__FUNCTION__, spurs.addr(), wid.addr(), pm.addr(), size, data, priorityTable.addr(), minContention, maxContention);

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

s32 _cellSpursWorkloadAttributeInitialize(
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
	cellSpurs.Warning("%s(attr_addr=0x%x, revision=%d, sdkVersion=0x%x, pm_addr=0x%x, size=0x%x, data=0x%llx, priorityTable_addr=0x%x, minContention=0x%x, maxContention=0x%x)",
		__FUNCTION__, attr.addr(), revision, sdkVersion, pm.addr(), size, data, priorityTable.addr(), minContention, maxContention);

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

s32 cellSpursWorkloadAttributeSetName(vm::ptr<CellSpursWorkloadAttribute> attr, vm::ptr<const char> nameClass, vm::ptr<const char> nameInstance)
{
	cellSpurs.Warning("%s(attr_addr=0x%x, nameClass_addr=0x%x, nameInstance_addr=0x%x)", __FUNCTION__, attr.addr(), nameClass.addr(), nameInstance.addr());

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

s32 cellSpursWorkloadAttributeSetShutdownCompletionEventHook(vm::ptr<CellSpursWorkloadAttribute> attr, vm::ptr<CellSpursShutdownCompletionEventHook> hook, vm::ptr<void> arg)
{
	cellSpurs.Warning("%s(attr_addr=0x%x, hook_addr=0x%x, arg=0x%x)", __FUNCTION__, attr.addr(), hook.addr(), arg.addr());

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

s32 cellSpursAddWorkloadWithAttribute(vm::ptr<CellSpurs> spurs, const vm::ptr<u32> wid, vm::ptr<const CellSpursWorkloadAttribute> attr)
{
	cellSpurs.Warning("%s(spurs_addr=0x%x, wid_addr=0x%x, attr_addr=0x%x)", __FUNCTION__, spurs.addr(), wid.addr(), attr.addr());

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

s32 cellSpursRemoveWorkload()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 cellSpursWaitForWorkloadShutdown()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 cellSpursShutdownWorkload()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 _cellSpursWorkloadFlagReceiver(vm::ptr<CellSpurs> spurs, u32 wid, u32 is_set)
{
	cellSpurs.Warning("%s(spurs_addr=0x%x, wid=%d, is_set=%d)", __FUNCTION__, spurs.addr(), wid, is_set);

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

s32 cellSpursGetWorkloadFlag(vm::ptr<CellSpurs> spurs, vm::ptr<vm::bptr<CellSpursWorkloadFlag>> flag)
{
	cellSpurs.Warning("%s(spurs_addr=0x%x, flag_addr=0x%x)", __FUNCTION__, spurs.addr(), flag.addr());

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

s32 cellSpursSendWorkloadSignal(vm::ptr<CellSpurs> spurs, u32 workloadId)
{
	cellSpurs.Warning("%s(spurs=0x%x, workloadId=0x%x)", __FUNCTION__, spurs.addr(), workloadId);

	if (spurs.addr() == 0)
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_NULL_POINTER;
	}

	if (spurs.addr() % CellSpurs::align)
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
}

s32 cellSpursGetWorkloadData(vm::ptr<CellSpurs> spurs, vm::ptr<u64> data, u32 workloadId)
{
	cellSpurs.Warning("%s(spurs_addr=0x%x, data=0x%x, workloadId=%d)", __FUNCTION__, spurs.addr(), data.addr(), workloadId);

	if (spurs.addr() == 0 || data.addr() == 0)
	{
		return CELL_SPURS_POLICY_MODULE_ERROR_NULL_POINTER;
	}

	if (spurs.addr() % CellSpurs::align)
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

	if (workloadId >= CELL_SPURS_MAX_WORKLOAD)
	{
		*data = spurs->m.wklInfo2[workloadId & 0x0F].arg;
	}
	else
	{
		*data = spurs->m.wklInfo1[workloadId].arg;
	}

	return CELL_OK;
}

s32 cellSpursReadyCountStore(vm::ptr<CellSpurs> spurs, u32 wid, u32 value)
{
	cellSpurs.Warning("%s(spurs_addr=0x%x, wid=%d, value=0x%x)", __FUNCTION__, spurs.addr(), wid, value);

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

s32 cellSpursReadyCountAdd()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 cellSpursReadyCountCompareAndSwap()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 cellSpursReadyCountSwap()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 cellSpursRequestIdleSpu()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 cellSpursGetWorkloadInfo()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 _cellSpursWorkloadFlagReceiver2()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 cellSpursSetExceptionEventHandler()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 cellSpursUnsetExceptionEventHandler()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 _cellSpursEventFlagInitialize(vm::ptr<CellSpurs> spurs, vm::ptr<CellSpursTaskset> taskset, vm::ptr<CellSpursEventFlag> eventFlag, u32 flagClearMode, u32 flagDirection)
{
	cellSpurs.Warning("_cellSpursEventFlagInitialize(spurs_addr=0x%x, taskset_addr=0x%x, eventFlag_addr=0x%x, flagClearMode=%d, flagDirection=%d)",
		spurs.addr(), taskset.addr(), eventFlag.addr(), flagClearMode, flagDirection);

	if (taskset.addr() == 0 && spurs.addr() == 0)
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
}

s32 cellSpursEventFlagAttachLv2EventQueue(vm::ptr<CellSpursEventFlag> eventFlag)
{
	cellSpurs.Warning("cellSpursEventFlagAttachLv2EventQueue(eventFlag_addr=0x%x)", eventFlag.addr());

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
}

s32 cellSpursEventFlagDetachLv2EventQueue(vm::ptr<CellSpursEventFlag> eventFlag)
{
	cellSpurs.Warning("cellSpursEventFlagDetachLv2EventQueue(eventFlag_addr=0x%x)", eventFlag.addr());

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

	s32 rc = CELL_OK;
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
}

s32 _cellSpursEventFlagWait(vm::ptr<CellSpursEventFlag> eventFlag, vm::ptr<u16> mask, u32 mode, u32 block)
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
		vm::var<sys_event_t> data;
		auto rc = sys_event_queue_receive(GetCurrentPPUThread(), eventFlag->m.eventQueueId, data, 0);
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

s32 cellSpursEventFlagWait(vm::ptr<CellSpursEventFlag> eventFlag, vm::ptr<u16> mask, u32 mode)
{
	cellSpurs.Warning("cellSpursEventFlagWait(eventFlag_addr=0x%x, mask_addr=0x%x, mode=%d)", eventFlag.addr(), mask.addr(), mode);

	return _cellSpursEventFlagWait(eventFlag, mask, mode, 1/*block*/);
}

s32 cellSpursEventFlagClear(vm::ptr<CellSpursEventFlag> eventFlag, u16 bits)
{
	cellSpurs.Warning("cellSpursEventFlagClear(eventFlag_addr=0x%x, bits=0x%x)", eventFlag.addr(), bits);

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
}

s32 cellSpursEventFlagSet(vm::ptr<CellSpursEventFlag> eventFlag, u16 bits)
{
	cellSpurs.Warning("cellSpursEventFlagSet(eventFlag_addr=0x%x, bits=0x%x)", eventFlag.addr(), bits);

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
}

s32 cellSpursEventFlagTryWait(vm::ptr<CellSpursEventFlag> eventFlag, vm::ptr<u16> mask, u32 mode)
{
	cellSpurs.Warning("cellSpursEventFlagTryWait(eventFlag_addr=0x%x, mask_addr=0x%x, mode=0x%x)", eventFlag.addr(), mask.addr(), mode);

	return _cellSpursEventFlagWait(eventFlag, mask, mode, 0/*block*/);
}

s32 cellSpursEventFlagGetDirection(vm::ptr<CellSpursEventFlag> eventFlag, vm::ptr<u32> direction)
{
	cellSpurs.Warning("cellSpursEventFlagGetDirection(eventFlag_addr=0x%x, direction_addr=0x%x)", eventFlag.addr(), direction.addr());

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
}

s32 cellSpursEventFlagGetClearMode(vm::ptr<CellSpursEventFlag> eventFlag, vm::ptr<u32> clear_mode)
{
	cellSpurs.Warning("cellSpursEventFlagGetClearMode(eventFlag_addr=0x%x, clear_mode_addr=0x%x)", eventFlag.addr(), clear_mode.addr());

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
}

s32 cellSpursEventFlagGetTasksetAddress(vm::ptr<CellSpursEventFlag> eventFlag, vm::ptr<CellSpursTaskset> taskset)
{
	cellSpurs.Warning("cellSpursEventFlagGetTasksetAddress(eventFlag_addr=0x%x, taskset_addr=0x%x)", eventFlag.addr(), taskset.addr());

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
}

s32 _cellSpursLFQueueInitialize()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 _cellSpursLFQueuePushBody()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 cellSpursLFQueueDetachLv2EventQueue()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 cellSpursLFQueueAttachLv2EventQueue()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 _cellSpursLFQueuePopBody()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 cellSpursLFQueueGetTasksetAddress()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 _cellSpursQueueInitialize()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 cellSpursQueuePopBody()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 cellSpursQueuePushBody()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 cellSpursQueueAttachLv2EventQueue()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 cellSpursQueueDetachLv2EventQueue()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 cellSpursQueueGetTasksetAddress()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 cellSpursQueueClear()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 cellSpursQueueDepth()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 cellSpursQueueGetEntrySize()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 cellSpursQueueSize()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 cellSpursQueueGetDirection()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 cellSpursCreateJobChainWithAttribute()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 cellSpursCreateJobChain()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 cellSpursJoinJobChain()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 cellSpursKickJobChain()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 _cellSpursJobChainAttributeInitialize()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 cellSpursGetJobChainId()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 cellSpursJobChainSetExceptionEventHandler()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 cellSpursJobChainUnsetExceptionEventHandler()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 cellSpursGetJobChainInfo()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 cellSpursJobChainGetSpursAddress()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 spursCreateTaskset(vm::ptr<CellSpurs> spurs, vm::ptr<CellSpursTaskset> taskset, u64 args, vm::ptr<const u8[8]> priority,
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
	_cellSpursWorkloadAttributeInitialize(wkl_attr, 1 /*revision*/, 0x33 /*sdk_version*/, vm::ptr<const void>::make(SPURS_IMG_ADDR_TASKSET_PM), 0x1E40 /*pm_size*/,
		taskset.addr(), priority, 8 /*min_contention*/, max_contention);
	// TODO: Check return code

	cellSpursWorkloadAttributeSetName(wkl_attr, vm::ptr<const char>::make(0), name);
	// TODO: Check return code

	// TODO: cellSpursWorkloadAttributeSetShutdownCompletionEventHook(wkl_attr, hook, taskset);
	// TODO: Check return code

	vm::var<be_t<u32>> wid;
	cellSpursAddWorkloadWithAttribute(spurs, vm::ptr<u32>::make(wid.addr()), vm::ptr<const CellSpursWorkloadAttribute>::make(wkl_attr.addr()));
	// TODO: Check return code

	taskset->m.wkl_flag_wait_task = 0x80;
	taskset->m.wid                = wid.value();
	// TODO: cellSpursSetExceptionEventHandler(spurs, wid, hook, taskset);
	// TODO: Check return code

	return CELL_OK;
}

s32 cellSpursCreateTasksetWithAttribute(vm::ptr<CellSpurs> spurs, vm::ptr<CellSpursTaskset> taskset, vm::ptr<CellSpursTasksetAttribute> attr)
{
	cellSpurs.Warning("%s(spurs=0x%x, taskset=0x%x, attr=0x%x)", __FUNCTION__, spurs.addr(), taskset.addr(), attr.addr());

	if (!attr)
	{
		// Syphurith: If that is intentionally done, please remove the 'return'. Or else just remove this comment.
		return CELL_SPURS_TASK_ERROR_NULL_POINTER;
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

s32 cellSpursCreateTaskset(vm::ptr<CellSpurs> spurs, vm::ptr<CellSpursTaskset> taskset, u64 args, vm::ptr<const u8[8]> priority, u32 maxContention)
{
	cellSpurs.Warning("cellSpursCreateTaskset(spurs_addr=0x%x, taskset_addr=0x%x, args=0x%llx, priority_addr=0x%x, maxContention=%d)",
		spurs.addr(), taskset.addr(), args, priority.addr(), maxContention);

	return spursCreateTaskset(spurs, taskset, args, priority, maxContention, vm::ptr<const char>::make(0), 6400/*CellSpursTaskset::size*/, 0);
}

s32 cellSpursJoinTaskset(vm::ptr<CellSpursTaskset> taskset)
{
	cellSpurs.Warning("cellSpursJoinTaskset(taskset_addr=0x%x)", taskset.addr());

	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 cellSpursGetTasksetId(vm::ptr<CellSpursTaskset> taskset, vm::ptr<u32> wid)
{
	cellSpurs.Warning("cellSpursGetTasksetId(taskset_addr=0x%x, wid=0x%x)", taskset.addr(), wid.addr());

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
}

s32 cellSpursShutdownTaskset(vm::ptr<CellSpursTaskset> taskset)
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

u32 _cellSpursGetSdkVersion()
{
	// Commenting this out for now since process_get_sdk_version does not return
	// the correct SDK version and instead returns a version too high for the game
	// and causes SPURS to fail.
	//s32 sdk_version;

	//if (process_get_sdk_version(process_getpid(), sdk_version) != CELL_OK)
	//{
	//	throw __FUNCTION__;
	//}

	//return sdk_version;
	return 1;
}

s32 spursCreateTask(vm::ptr<CellSpursTaskset> taskset, vm::ptr<u32> task_id, vm::ptr<u32> elf_addr, vm::ptr<u32> context_addr, u32 context_size, vm::ptr<CellSpursTaskLsPattern> ls_pattern, vm::ptr<CellSpursTaskArgument> arg)
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
			u128 ls_pattern_128 = u128::from64r(ls_pattern->_u64[0], ls_pattern->_u64[1]);
			u32 ls_blocks       = 0;
			for (auto i = 0; i < 128; i++)
			{
				if (ls_pattern_128._bit[i])
				{
					ls_blocks++;
				}
			}

			if (ls_blocks > alloc_ls_blocks)
			{
				return CELL_SPURS_TASK_ERROR_INVAL;
			}

			u128 _0 = u128::from32(0);
			if ((ls_pattern_128 & u128::from32r(0xFC000000)) != _0)
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
		if (!taskset->m.enabled.value()._bit[tmp_task_id])
		{
			auto enabled              = taskset->m.enabled.value();
			enabled._bit[tmp_task_id] = true;
			taskset->m.enabled        = enabled;
			break;
		}
	}

	if (tmp_task_id >= CELL_SPURS_MAX_TASK)
	{
		// Syphurith: If that is intentionally done, please remove the 'return'. Or else just remove this comment.
		return CELL_SPURS_TASK_ERROR_AGAIN;
	}

	taskset->m.task_info[tmp_task_id].elf_addr.set(elf_addr.addr());
	taskset->m.task_info[tmp_task_id].context_save_storage_and_alloc_ls_blocks = (context_addr.addr() | alloc_ls_blocks);
	taskset->m.task_info[tmp_task_id].args                                     = *arg;
	if (ls_pattern.addr())
	{
		taskset->m.task_info[tmp_task_id].ls_pattern = *ls_pattern;
	}

	*task_id = tmp_task_id;
	return CELL_OK;
}

s32 spursTaskStart(vm::ptr<CellSpursTaskset> taskset, u32 taskId)
{
	auto pendingReady         = taskset->m.pending_ready.value();
	pendingReady._bit[taskId] = true;
	taskset->m.pending_ready  = pendingReady;

	cellSpursSendWorkloadSignal(vm::ptr<CellSpurs>::make((u32)taskset->m.spurs.addr()), taskset->m.wid);
	auto rc = cellSpursWakeUp(GetCurrentPPUThread(), vm::ptr<CellSpurs>::make((u32)taskset->m.spurs.addr()));
	if (rc != CELL_OK)
	{
		if (rc == CELL_SPURS_POLICY_MODULE_ERROR_STAT)
		{
			rc = CELL_SPURS_TASK_ERROR_STAT;
		}
		else
		{
			assert(0);
		}
	}

	return rc;
}

s32 cellSpursCreateTask(vm::ptr<CellSpursTaskset> taskset, vm::ptr<u32> taskId, u32 elf_addr, u32 context_addr, u32 context_size, vm::ptr<CellSpursTaskLsPattern> lsPattern,
	vm::ptr<CellSpursTaskArgument> argument)
{
	cellSpurs.Warning("cellSpursCreateTask(taskset_addr=0x%x, taskID_addr=0x%x, elf_addr_addr=0x%x, context_addr_addr=0x%x, context_size=%d, lsPattern_addr=0x%x, argument_addr=0x%x)",
		taskset.addr(), taskId.addr(), elf_addr, context_addr, context_size, lsPattern.addr(), argument.addr());

	if (!taskset)
	{
		return CELL_SPURS_TASK_ERROR_NULL_POINTER;
	}

	if (taskset.addr() % CellSpursTaskset::align)
	{
		return CELL_SPURS_TASK_ERROR_ALIGN;
	}

	vm::var<be_t<u32>> tmpTaskId;
	auto rc = spursCreateTask(taskset, tmpTaskId, vm::ptr<u32>::make(elf_addr), vm::ptr<u32>::make(context_addr), context_size, lsPattern, argument);
	if (rc != CELL_OK) 
	{
		return rc;
	}

	rc = spursTaskStart(taskset, tmpTaskId->value());
	if (rc != CELL_OK) 
	{
		return rc;
	}

	*taskId = tmpTaskId;
	return CELL_OK;
}

s32 _cellSpursSendSignal(vm::ptr<CellSpursTaskset> taskset, u32 taskId)
{
	if (!taskset)
	{
		return CELL_SPURS_TASK_ERROR_NULL_POINTER;
	}

	if (taskset.addr() % CellSpursTaskset::align)
	{
		return CELL_SPURS_TASK_ERROR_ALIGN;
	}

	if (taskId >= CELL_SPURS_MAX_TASK || taskset->m.wid >= CELL_SPURS_MAX_WORKLOAD2)
	{
		return CELL_SPURS_TASK_ERROR_INVAL;
	}

	auto _0       = be_t<u128>::make(u128::from32(0));
	auto disabled = taskset->m.enabled.value()._bit[taskId] ? false : true;
	auto invalid  = (taskset->m.ready & taskset->m.pending_ready) != _0 || (taskset->m.running & taskset->m.waiting) != _0 || disabled ||
					((taskset->m.running | taskset->m.ready | taskset->m.pending_ready | taskset->m.waiting | taskset->m.signalled) & be_t<u128>::make(~taskset->m.enabled.value())) != _0;

	if (invalid)
	{
		return CELL_SPURS_TASK_ERROR_SRCH;
	}

	auto shouldSignal      = (taskset->m.waiting & be_t<u128>::make(~taskset->m.signalled.value()) & be_t<u128>::make(u128::fromBit(taskId))) != _0 ? true : false;
	auto signalled         = taskset->m.signalled.value();
	signalled._bit[taskId] = true;
	taskset->m.signalled   = signalled;
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
}

s32 cellSpursCreateTaskWithAttribute()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 cellSpursTasksetAttributeSetName(vm::ptr<CellSpursTasksetAttribute> attr, vm::ptr<const char> name)
{
	cellSpurs.Warning("%s(attr=0x%x, name=0x%x)", __FUNCTION__, attr.addr(), name.addr());

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
}

s32 cellSpursTasksetAttributeSetTasksetSize(vm::ptr<CellSpursTasksetAttribute> attr, u32 size)
{
	cellSpurs.Warning("%s(attr=0x%x, size=0x%x)", __FUNCTION__, attr.addr(), size);

	if (!attr)
	{
		return CELL_SPURS_TASK_ERROR_NULL_POINTER;
	}

	if (attr.addr() % CellSpursTasksetAttribute::align)
	{
		return CELL_SPURS_TASK_ERROR_ALIGN;
	}

	if (size != 6400/*CellSpursTaskset::size*/ && size != CellSpursTaskset2::size)
	{
		return CELL_SPURS_TASK_ERROR_INVAL;
	}

	attr->m.taskset_size = size;
	return CELL_OK;
}

s32 cellSpursTasksetAttributeEnableClearLS(vm::ptr<CellSpursTasksetAttribute> attr, s32 enable)
{
	cellSpurs.Warning("%s(attr=0x%x, enable=%d)", __FUNCTION__, attr.addr(), enable);

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
}

s32 _cellSpursTasksetAttribute2Initialize(vm::ptr<CellSpursTasksetAttribute2> attribute, u32 revision)
{
	cellSpurs.Warning("_cellSpursTasksetAttribute2Initialize(attribute_addr=0x%x, revision=%d)", attribute.addr(), revision);

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
}

s32 cellSpursTaskExitCodeGet()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 cellSpursTaskExitCodeInitialize()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 cellSpursTaskExitCodeTryGet()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 cellSpursTaskGetLoadableSegmentPattern()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 cellSpursTaskGetReadOnlyAreaPattern()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 cellSpursTaskGenerateLsPattern()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 _cellSpursTaskAttributeInitialize()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 cellSpursTaskAttributeSetExitCodeContainer()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 _cellSpursTaskAttribute2Initialize(vm::ptr<CellSpursTaskAttribute2> attribute, u32 revision)
{
	cellSpurs.Warning("_cellSpursTaskAttribute2Initialize(attribute_addr=0x%x, revision=%d)", attribute.addr(), revision);

	attribute->revision = revision;
	attribute->sizeContext = 0;
	attribute->eaContext = 0;

	for (s32 c = 0; c < 4; c++)
	{
		attribute->lsPattern._u32[c] = 0;
	}

	attribute->name_addr = 0;

	return CELL_OK;
}

s32 cellSpursTaskGetContextSaveAreaSize()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 cellSpursCreateTaskset2(vm::ptr<CellSpurs> spurs, vm::ptr<CellSpursTaskset2> taskset, vm::ptr<CellSpursTasksetAttribute2> attr)
{
	cellSpurs.Warning("%s(spurs=0x%x, taskset=0x%x, attr=0x%x)", __FUNCTION__, spurs.addr(), taskset.addr(), attr.addr());

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
}

s32 cellSpursCreateTask2()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 cellSpursJoinTask2()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 cellSpursTryJoinTask2()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 cellSpursDestroyTaskset2()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 cellSpursCreateTask2WithBinInfo()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 cellSpursTasksetSetExceptionEventHandler(vm::ptr<CellSpursTaskset> taskset, vm::ptr<u64> handler, vm::ptr<u64> arg)
{
	cellSpurs.Warning("%s(taskset=0x5x, handler=0x%x, arg=0x%x)", __FUNCTION__, taskset.addr(), handler.addr(), arg.addr());

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
}

s32 cellSpursTasksetUnsetExceptionEventHandler(vm::ptr<CellSpursTaskset> taskset)
{
	cellSpurs.Warning("%s(taskset=0x%x)", __FUNCTION__, taskset.addr());

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
}

s32 cellSpursLookUpTasksetAddress(vm::ptr<CellSpurs> spurs, vm::ptr<CellSpursTaskset> taskset, u32 id)
{
	cellSpurs.Warning("%s(spurs=0x%x, taskset=0x%x, id=0x%x)", __FUNCTION__, spurs.addr(), taskset.addr(), id);

	if (taskset.addr() == 0)
	{
		return CELL_SPURS_TASK_ERROR_NULL_POINTER;
	}

	vm::var<be_t<u64>> data;
	auto rc = cellSpursGetWorkloadData(spurs, vm::ptr<u64>::make(data.addr()), id);
	if (rc != CELL_OK)
	{
		// Convert policy module error code to a task error code
		return rc ^ 0x100;
	}

	taskset.set((u32)data.value());
	return CELL_OK;
}

s32 cellSpursTasksetGetSpursAddress(vm::ptr<const CellSpursTaskset> taskset, vm::ptr<u32> spurs)
{
	cellSpurs.Warning("%s(taskset=0x%x, spurs=0x%x)", __FUNCTION__, taskset.addr(), spurs.addr());

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
}

s32 cellSpursGetTasksetInfo()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 _cellSpursTasksetAttributeInitialize(vm::ptr<CellSpursTasksetAttribute> attribute, u32 revision, u32 sdk_version, u64 args, vm::ptr<const u8> priority, u32 max_contention)
{
	cellSpurs.Warning("%s(attribute=0x%x, revision=%d, skd_version=%d, args=0x%llx, priority=0x%x, max_contention=%d)",
		__FUNCTION__, attribute.addr(), revision, sdk_version, args, priority.addr(), max_contention);

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
	attribute->m.taskset_size = 6400/*CellSpursTaskset::size*/;
	attribute->m.max_contention = max_contention;
	return CELL_OK;
}

s32 cellSpursJobGuardInitialize()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 cellSpursJobChainAttributeSetName()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 cellSpursShutdownJobChain()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 cellSpursJobChainAttributeSetHaltOnError()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 cellSpursJobChainAttributeSetJobTypeMemoryCheck()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 cellSpursJobGuardNotify()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 cellSpursJobGuardReset()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 cellSpursRunJobChain()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 cellSpursJobChainGetError()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 cellSpursGetJobPipelineInfo()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 cellSpursJobSetMaxGrab()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 cellSpursJobHeaderSetJobbin2Param()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 cellSpursAddUrgentCommand()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 cellSpursAddUrgentCall()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 cellSpursBarrierInitialize()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 cellSpursBarrierGetTasksetAddress()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 _cellSpursSemaphoreInitialize()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

s32 cellSpursSemaphoreGetTasksetAddress()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

bool spursIsLibProfLoaded()
{
	return false;
}

void spursTraceStatusUpdate(vm::ptr<CellSpurs> spurs)
{
	LV2_LOCK;

	if (spurs->m.xCC != 0)
	{
		spurs->m.xCD                  = 1;
		spurs->m.sysSrvMsgUpdateTrace = (1 << spurs->m.nSpus) - 1;
		spurs->m.sysSrvMessage.write_relaxed(0xFF);
		sys_semaphore_wait((u32)spurs->m.semPrv, 0);
	}
}

s32 spursTraceInitialize(vm::ptr<CellSpurs> spurs, vm::ptr<CellSpursTraceInfo> buffer, u32 size, u32 mode, u32 updateStatus)
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

s32 cellSpursTraceInitialize(vm::ptr<CellSpurs> spurs, vm::ptr<CellSpursTraceInfo> buffer, u32 size, u32 mode)
{
	if (spursIsLibProfLoaded())
	{
		return CELL_SPURS_CORE_ERROR_STAT;
	}

	return spursTraceInitialize(spurs, buffer, size, mode, 1);
}

s32 spursTraceStart(vm::ptr<CellSpurs> spurs, u32 updateStatus)
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

s32 cellSpursTraceStart(vm::ptr<CellSpurs> spurs)
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

s32 spursTraceStop(vm::ptr<CellSpurs> spurs, u32 updateStatus)
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

s32 cellSpursTraceStop(vm::ptr<CellSpurs> spurs)
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

s32 cellSpursTraceFinalize(vm::ptr<CellSpurs> spurs)
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

Module cellSpurs("cellSpurs", []()
{
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
});
