#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/SysCalls/Modules.h"

#include "Emu/Cell/SPURSManager.h"
#include "cellSpurs.h"

//void cellSpurs_init();
//Module cellSpurs(0x000a, cellSpurs_init);
Module *cellSpurs = nullptr;

#ifdef _WIN32
extern u32 libsre;
extern u32 libsre_rtoc;
#endif

s64 cellSpursInitialize(mem_ptr_t<CellSpurs> spurs, s32 nSpus, s32 spuPriority, s32 ppuPriority, bool exitIfNoWork)
{
	cellSpurs->Warning("cellSpursInitialize(spurs_addr=0x%x, nSpus=%d, spuPriority=%d, ppuPriority=%d, exitIfNoWork=%d)", spurs.GetAddr(), nSpus, spuPriority, ppuPriority, exitIfNoWork);

#ifdef PRX_DEBUG
	return GetCurrentPPUThread().FastCall2(libsre + 0x8480, libsre_rtoc);
#else
	SPURSManagerAttribute *attr = new SPURSManagerAttribute(nSpus, spuPriority, ppuPriority, exitIfNoWork);
	spurs->spurs = new SPURSManager(attr);

	return CELL_OK;
#endif
}

s64 cellSpursFinalize(mem_ptr_t<CellSpurs> spurs)
{
	cellSpurs->Warning("cellSpursFinalize(spurs_addr=0x%x)", spurs.GetAddr());

#ifdef PRX_DEBUG
	return GetCurrentPPUThread().FastCall2(libsre + 0x8568, libsre_rtoc);
#else
	spurs->spurs->Finalize();

	return CELL_OK;
#endif
}

s64 cellSpursInitializeWithAttribute(mem_ptr_t<CellSpurs> spurs, const mem_ptr_t<CellSpursAttribute> attr)
{
	cellSpurs->Warning("cellSpursInitializeWithAttribute(spurs_addr=0x%x, spurs_addr=0x%x)", spurs.GetAddr(), attr.GetAddr());

#ifdef PRX_DEBUG
	return GetCurrentPPUThread().FastCall2(libsre + 0x839C, libsre_rtoc);
#else
	spurs->spurs = new SPURSManager(attr->attr);

	return CELL_OK;
#endif
}

s64 cellSpursInitializeWithAttribute2(mem_ptr_t<CellSpurs2> spurs, const mem_ptr_t<CellSpursAttribute> attr)
{
	cellSpurs->Warning("cellSpursInitializeWithAttribute2(spurs_addr=0x%x, spurs_addr=0x%x)", spurs.GetAddr(), attr.GetAddr());

#ifdef PRX_DEBUG
	return GetCurrentPPUThread().FastCall2(libsre + 0x82B4, libsre_rtoc);
#else
	spurs->spurs = new SPURSManager(attr->attr);

	return CELL_OK;
#endif
}

s64 _cellSpursAttributeInitialize(mem_ptr_t<CellSpursAttribute> attr, s32 nSpus, s32 spuPriority, s32 ppuPriority, bool exitIfNoWork)
{
	cellSpurs->Warning("_cellSpursAttributeInitialize(attr_addr=0x%x, nSpus=%d, spuPriority=%d, ppuPriority=%d, exitIfNoWork=%d)",
		attr.GetAddr(), nSpus, spuPriority, ppuPriority, exitIfNoWork);

#ifdef PRX_DEBUG
	return GetCurrentPPUThread().FastCall2(libsre + 0x72CC, libsre_rtoc);
#else
	attr->attr = new SPURSManagerAttribute(nSpus, spuPriority, ppuPriority, exitIfNoWork);

	return CELL_OK;
#endif
}

s64 cellSpursAttributeSetMemoryContainerForSpuThread(mem_ptr_t<CellSpursAttribute> attr, u32 container)
{
	cellSpurs->Warning("cellSpursAttributeSetMemoryContainerForSpuThread(attr_addr=0x%x, container=0x%x)", attr.GetAddr(), container);

#ifdef PRX_DEBUG
	return GetCurrentPPUThread().FastCall2(libsre + 0x6FF8, libsre_rtoc);
#else
	attr->attr->_setMemoryContainerForSpuThread(container);

	return CELL_OK;
#endif
}

s64 cellSpursAttributeSetNamePrefix(mem_ptr_t<CellSpursAttribute> attr, vm::ptr<const char> prefix, u32 size)
{
	cellSpurs->Warning("cellSpursAttributeSetNamePrefix(attr_addr=0x%x, prefix_addr=0x%x, size=0x%x)", attr.GetAddr(), prefix.addr(), size);

#ifdef PRX_DEBUG
	return GetCurrentPPUThread().FastCall2(libsre + 0x7234, libsre_rtoc);
#else
	if (size > CELL_SPURS_NAME_MAX_LENGTH)
	{
		cellSpurs->Error("cellSpursAttributeSetNamePrefix : CELL_SPURS_CORE_ERROR_INVAL");
		return CELL_SPURS_CORE_ERROR_INVAL;
	}

	attr->attr->_setNamePrefix(prefix.get_ptr(), size);

	return CELL_OK;
#endif
}

s64 cellSpursAttributeEnableSpuPrintfIfAvailable(mem_ptr_t<CellSpursAttribute> attr)
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("cellSpursAttributeEnableSpuPrintfIfAvailable(attr_addr=0x%x)", attr.GetAddr());
	return GetCurrentPPUThread().FastCall2(libsre + 0x7150, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursAttributeSetSpuThreadGroupType(mem_ptr_t<CellSpursAttribute> attr, s32 type)
{
	cellSpurs->Warning("cellSpursAttributeSetSpuThreadGroupType(attr_addr=0x%x, type=%d)", attr.GetAddr(), type);

#ifdef PRX_DEBUG
	return GetCurrentPPUThread().FastCall2(libsre + 0x70C8, libsre_rtoc);
#else
	attr->attr->_setSpuThreadGroupType(type);

	return CELL_OK;
#endif
}

s64 cellSpursAttributeEnableSystemWorkload(mem_ptr_t<CellSpursAttribute> attr, vm::ptr<const u8> priority, u32 maxSpu, vm::ptr<const bool> isPreemptible)
{
	cellSpurs->Warning("cellSpursAttributeEnableSystemWorkload(attr_addr=0x%x, priority_addr=0x%x, maxSpu=%d, isPreemptible_addr=0x%x)",
		attr.GetAddr(), priority.addr(), maxSpu, isPreemptible.addr());

#ifdef PRX_DEBUG
	return GetCurrentPPUThread().FastCall2(libsre + 0xF410, libsre_rtoc);
#else
	for (s32 i = 0; i < CELL_SPURS_MAX_SPU; i++)
	{
		if (priority[i] != 1 || maxSpu == 0)
		{
			cellSpurs->Error("cellSpursAttributeEnableSystemWorkload : CELL_SPURS_CORE_ERROR_INVAL");
			return CELL_SPURS_CORE_ERROR_INVAL;
		}
	}
	return CELL_OK;
#endif
}

s64 cellSpursGetSpuThreadGroupId(mem_ptr_t<CellSpurs> spurs, vm::ptr<be_t<u32>> group)
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("cellSpursGetSpuThreadGroupId(spurs_addr=0x%x, group_addr=0x%x)", spurs.GetAddr(), group.addr());
	return GetCurrentPPUThread().FastCall2(libsre + 0x8B30, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursGetNumSpuThread(mem_ptr_t<CellSpurs> spurs, vm::ptr<be_t<u32>> nThreads)
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("cellSpursGetNumSpuThread(spurs_addr=0x%x, nThreads_addr=0x%x)", spurs.GetAddr(), nThreads.addr());
	return GetCurrentPPUThread().FastCall2(libsre + 0x8B78, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursGetSpuThreadId(mem_ptr_t<CellSpurs> spurs, vm::ptr<be_t<u32>> thread, vm::ptr<be_t<u32>> nThreads)
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("cellSpursGetSpuThreadId(spurs_addr=0x%x, thread_addr=0x%x, nThreads_addr=0x%x)", spurs.GetAddr(), thread.addr(), nThreads.addr());
	return GetCurrentPPUThread().FastCall2(libsre + 0x8A98, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursSetMaxContention(mem_ptr_t<CellSpurs> spurs, u32 workloadId, u32 maxContention)
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("cellSpursSetMaxContention(spurs_addr=0x%x, workloadId=%d, maxContention=%d)", spurs.GetAddr(), workloadId, maxContention);
	return GetCurrentPPUThread().FastCall2(libsre + 0x8E90, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursSetPriorities(mem_ptr_t<CellSpurs> spurs, u32 workloadId, vm::ptr<const u8> priorities)
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("cellSpursSetPriorities(spurs_addr=0x%x, workloadId=%d, priorities_addr=0x%x)", spurs.GetAddr(), workloadId, priorities.addr());
	return GetCurrentPPUThread().FastCall2(libsre + 0x8BC0, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursSetPreemptionVictimHints(mem_ptr_t<CellSpurs> spurs, vm::ptr<const bool> isPreemptible)
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("cellSpursSetPreemptionVictimHints(spurs_addr=0x%x, isPreemptible_addr=0x%x)", spurs.GetAddr(), isPreemptible.addr());
	return GetCurrentPPUThread().FastCall2(libsre + 0xF5A4, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursAttachLv2EventQueue(mem_ptr_t<CellSpurs> spurs, u32 queue, vm::ptr<u8> port, s32 isDynamic)
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("cellSpursAttachLv2EventQueue(spurs_addr=0x%x, queue=%d, port_addr=0x%x, isDynamic=%d)",
		spurs.GetAddr(), queue, port.addr(), isDynamic);
	return GetCurrentPPUThread().FastCall2(libsre + 0xAFE0, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursDetachLv2EventQueue(mem_ptr_t<CellSpurs> spurs, u8 port)
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("cellSpursDetachLv2EventQueue(spurs_addr=0x%x, port=0x%x)", spurs.GetAddr(), port);
	return GetCurrentPPUThread().FastCall2(libsre + 0xB144, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursEnableExceptionEventHandler(mem_ptr_t<CellSpurs> spurs, bool flag)
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("cellSpursEnableExceptionEventHandler(spurs_addr=0x%x, flag=%d)", spurs.GetAddr(), flag);
	return GetCurrentPPUThread().FastCall2(libsre + 0xDCC0, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursSetGlobalExceptionEventHandler(mem_ptr_t<CellSpurs> spurs, u32 eaHandler_addr, u32 arg_addr)
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("cellSpursSetGlobalExceptionEventHandler(spurs_addr=0x%x, eaHandler_addr=0x%x, arg_addr=0x%x)",
		spurs.GetAddr(), eaHandler_addr, arg_addr);
	return GetCurrentPPUThread().FastCall2(libsre + 0xD6D0, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursUnsetGlobalExceptionEventHandler(mem_ptr_t<CellSpurs> spurs)
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("cellSpursUnsetGlobalExceptionEventHandler(spurs_addr=0x%x)", spurs.GetAddr());
	return GetCurrentPPUThread().FastCall2(libsre + 0xD674, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursGetInfo(mem_ptr_t<CellSpurs> spurs, mem_ptr_t<CellSpursInfo> info)
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("cellSpursGetInfo(spurs_addr=0x%x, info_addr=0x%x)", spurs.GetAddr(), info.GetAddr());
	return GetCurrentPPUThread().FastCall2(libsre + 0xE540, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 _cellSpursEventFlagInitialize(mem_ptr_t<CellSpurs> spurs, mem_ptr_t<CellSpursTaskset> taskset, mem_ptr_t<CellSpursEventFlag> eventFlag, u32 flagClearMode, u32 flagDirection)
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("_cellSpursEventFlagInitialize(spurs_addr=0x%x, taskset_addr=0x%x, eventFlag_addr=0x%x, flagClearMode=%d, flagDirection=%d)",
		spurs.GetAddr(), taskset.GetAddr(), eventFlag.GetAddr(), flagClearMode, flagDirection);
	return GetCurrentPPUThread().FastCall2(libsre + 0x1564C, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursEventFlagAttachLv2EventQueue(mem_ptr_t<CellSpursEventFlag> eventFlag)
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("cellSpursEventFlagAttachLv2EventQueue(eventFlag_addr=0x%x)", eventFlag.GetAddr());
	return GetCurrentPPUThread().FastCall2(libsre + 0x157B8, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursEventFlagDetachLv2EventQueue(mem_ptr_t<CellSpursEventFlag> eventFlag)
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("cellSpursEventFlagDetachLv2EventQueue(eventFlag_addr=0x%x)", eventFlag.GetAddr());
	return GetCurrentPPUThread().FastCall2(libsre + 0x15998, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursEventFlagWait(mem_ptr_t<CellSpursEventFlag> eventFlag, vm::ptr<be_t<u16>> mask, u32 mode)
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("cellSpursEventFlagWait(eventFlag_addr=0x%x, mask_addr=0x%x, mode=%d)", eventFlag.GetAddr(), mask.addr(), mode);
	return GetCurrentPPUThread().FastCall2(libsre + 0x15E68, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursEventFlagClear(mem_ptr_t<CellSpursEventFlag> eventFlag, u16 bits)
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("cellSpursEventFlagClear(eventFlag_addr=0x%x, bits=0x%x)", eventFlag.GetAddr(), bits);
	return GetCurrentPPUThread().FastCall2(libsre + 0x15E9C, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursEventFlagSet(mem_ptr_t<CellSpursEventFlag> eventFlag, u16 bits)
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("cellSpursEventFlagSet(eventFlag_addr=0x%x, bits=0x%x)", eventFlag.GetAddr(), bits);
	return GetCurrentPPUThread().FastCall2(libsre + 0x15F04, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursEventFlagTryWait(mem_ptr_t<CellSpursEventFlag> eventFlag, vm::ptr<be_t<u16>> mask, u32 mode)
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("cellSpursEventFlagTryWait(eventFlag_addr=0x%x, mask_addr=0x%x, mode=0x%x)", eventFlag.GetAddr(), mask.addr(), mode);
	return GetCurrentPPUThread().FastCall2(libsre + 0x15E70, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursEventFlagGetDirection(mem_ptr_t<CellSpursEventFlag> eventFlag, vm::ptr<be_t<u32>> direction)
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("cellSpursEventFlagGetDirection(eventFlag_addr=0x%x, direction_addr=0x%x)", eventFlag.GetAddr(), direction.addr());
	return GetCurrentPPUThread().FastCall2(libsre + 0x162C4, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursEventFlagGetClearMode(mem_ptr_t<CellSpursEventFlag> eventFlag, vm::ptr<be_t<u32>> clear_mode)
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("cellSpursEventFlagGetClearMode(eventFlag_addr=0x%x, clear_mode_addr=0x%x)", eventFlag.GetAddr(), clear_mode.addr());
	return GetCurrentPPUThread().FastCall2(libsre + 0x16310, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursEventFlagGetTasksetAddress(mem_ptr_t<CellSpursEventFlag> eventFlag, mem_ptr_t<CellSpursTaskset> taskset)
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("cellSpursEventFlagGetTasksetAddress(eventFlag_addr=0x%x, taskset_addr=0x%x)", eventFlag.GetAddr(), taskset.GetAddr());
	return GetCurrentPPUThread().FastCall2(libsre + 0x1635C, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
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

s64 cellSpursCreateTasksetWithAttribute()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x14BEC, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursCreateTaskset(mem_ptr_t<CellSpurs> spurs, mem_ptr_t<CellSpursTaskset> taskset, u64 args, vm::ptr<const u8> priority, u32 maxContention)
{
	cellSpurs->Warning("cellSpursCreateTaskset(spurs_addr=0x%x, taskset_addr=0x%x, args=0x%llx, priority_addr=0x%x, maxContention=%d)",
		spurs.GetAddr(), taskset.GetAddr(), args, priority.addr(), maxContention);

#ifdef PRX_DEBUG
	return GetCurrentPPUThread().FastCall2(libsre + 0x14CB8, libsre_rtoc);
#else
	SPURSManagerTasksetAttribute *tattr = new SPURSManagerTasksetAttribute(args, priority, maxContention);
	taskset->taskset = new SPURSManagerTaskset(taskset.GetAddr(), tattr);

	return CELL_OK;
#endif
}

s64 cellSpursJoinTaskset(mem_ptr_t<CellSpursTaskset> taskset)
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("cellSpursJoinTaskset(taskset_addr=0x%x)", taskset.GetAddr());
	return GetCurrentPPUThread().FastCall2(libsre + 0x152F8, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursGetTasksetId(mem_ptr_t<CellSpursTaskset> taskset, vm::ptr<be_t<u32>> workloadId)
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("cellSpursGetTasksetId(taskset_addr=0x%x, workloadId_addr=0x%x)", taskset.GetAddr(), workloadId.addr());
	return GetCurrentPPUThread().FastCall2(libsre + 0x14EA0, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursShutdownTaskset(mem_ptr_t<CellSpursTaskset> taskset)
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("cellSpursShutdownTaskset(taskset_addr=0x%x)", taskset.GetAddr());
	return GetCurrentPPUThread().FastCall2(libsre + 0x14868, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursCreateTask(mem_ptr_t<CellSpursTaskset> taskset, vm::ptr<be_t<u32>> taskID, u32 elf_addr, u32 context_addr, u32 context_size, mem_ptr_t<CellSpursTaskLsPattern> lsPattern,
	mem_ptr_t<CellSpursTaskArgument> argument)
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("cellSpursCreateTask(taskset_addr=0x%x, taskID_addr=0x%x, elf_addr_addr=0x%x, context_addr_addr=0x%x, context_size=%d, lsPattern_addr=0x%x, argument_addr=0x%x)",
		taskset.GetAddr(), taskID.addr(), elf_addr, context_addr, context_size, lsPattern.GetAddr(), argument.GetAddr());
	return GetCurrentPPUThread().FastCall2(libsre + 0x12414, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 _cellSpursSendSignal(mem_ptr_t<CellSpursTaskset> taskset, u32 taskID)
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("_cellSpursSendSignal(taskset_addr=0x%x, taskID=%d)", taskset.GetAddr(), taskID);
	return GetCurrentPPUThread().FastCall2(libsre + 0x124CC, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursCreateTaskWithAttribute()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x12204, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursTasksetAttributeSetName()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x14210, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursTasksetAttributeSetTasksetSize()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x14254, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursTasksetAttributeEnableClearLS()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x142AC, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 _cellSpursTasksetAttribute2Initialize(mem_ptr_t<CellSpursTasksetAttribute2> attribute, u32 revision)
{
	cellSpurs->Warning("_cellSpursTasksetAttribute2Initialize(attribute_addr=0x%x, revision=%d)", attribute.GetAddr(), revision);
	
#ifdef PRX_DEBUG
	return GetCurrentPPUThread().FastCall2(libsre + 0x1474C, libsre_rtoc);
#else
	attribute->revision = revision;
	attribute->name_addr = NULL;
	attribute->argTaskset = 0;

	for (s32 i = 0; i < 8; i++)
	{
		attribute->priority[i] = 1;
	}

	attribute->maxContention = 8;
	attribute->enableClearLs = 0;
	attribute->CellSpursTaskNameBuffer_addr = 0;

	return CELL_OK;
#endif
}

s64 cellSpursTaskExitCodeGet()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x1397C, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursTaskExitCodeInitialize()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x1352C, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursTaskExitCodeTryGet()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x13974, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursTaskGetLoadableSegmentPattern()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x13ED4, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursTaskGetReadOnlyAreaPattern()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x13CFC, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursTaskGenerateLsPattern()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x13B78, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 _cellSpursTaskAttributeInitialize()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x10C30, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursTaskAttributeSetExitCodeContainer()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x10A98, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 _cellSpursTaskAttribute2Initialize(mem_ptr_t<CellSpursTaskAttribute2> attribute, u32 revision)
{
	cellSpurs->Warning("_cellSpursTaskAttribute2Initialize(attribute_addr=0x%x, revision=%d)", attribute.GetAddr(), revision);

#ifdef PRX_DEBUG
	return GetCurrentPPUThread().FastCall2(libsre + 0x10B00, libsre_rtoc);
#else
	attribute->revision = revision;
	attribute->sizeContext = 0;
	attribute->eaContext = NULL;
	
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
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x1409C, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursCreateTaskset2()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x15108, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursCreateTask2()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x11E54, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursJoinTask2()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x11378, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursTryJoinTask2()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x11748, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursDestroyTaskset2()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x14EE8, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursCreateTask2WithBinInfo()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x120E0, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursTasksetSetExceptionEventHandler()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x13124, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursTasksetUnsetExceptionEventHandler()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x13194, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursLookUpTasksetAddress()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x133AC, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursTasksetGetSpursAddress()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x14408, libsre_rtoc);
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

s64 cellSpursGetTasksetInfo()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x1445C, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 _cellSpursTasksetAttributeInitialize()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x142FC, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
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

s64 cellSpursWorkloadAttributeSetName()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x9664, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursWorkloadAttributeSetShutdownCompletionEventHook()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x96A4, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursAddWorkloadWithAttribute()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x9E14, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
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

s64 cellSpursAddWorkload()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x9ED0, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursWakeUp()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x84D8, libsre_rtoc);
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

s64 _cellSpursWorkloadFlagReceiver()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0xF158, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursGetWorkloadFlag()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0xEC00, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursReadyCountStore()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0xAB2C, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 _cellSpursWorkloadAttributeInitialize()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0x9F08, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
#endif
}

s64 cellSpursSendWorkloadSignal()
{
#ifdef PRX_DEBUG
	cellSpurs->Warning("%s()", __FUNCTION__);
	return GetCurrentPPUThread().FastCall2(libsre + 0xA658, libsre_rtoc);
#else
	UNIMPLEMENTED_FUNC(cellSpurs);
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

void cellSpurs_init()
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
	REG_FUNC(cellSpurs, cellSpursSetExceptionEventHandler);
	REG_FUNC(cellSpurs, cellSpursUnsetExceptionEventHandler);
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

	// TODO: some trace funcs
}