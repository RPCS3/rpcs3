#include "stdafx.h"
#include "Emu/ConLog.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/Cell/PPUThread.h"
#include "Emu/SysCalls/SC_FUNC.h"
#include "Emu/SysCalls/Modules.h"

#include "cellSpurs.h"

//void cellSpurs_init();
//Module cellSpurs(0x000a, cellSpurs_init);
Module *cellSpurs = nullptr;

int cellSpursInitialize(mem_ptr_t<CellSpurs> spurs, int nSpus, int spuPriority,
	int ppuPriority, bool exitIfNoWork)
{
	cellSpurs->Warning("cellSpursInitialize(spurs_addr=0x%x, nSpus=%u, spuPriority=%u, ppuPriority=%u, exitIfNoWork=%u)", spurs.GetAddr(), nSpus, spuPriority, ppuPriority, exitIfNoWork);

	if (spurs.GetAddr() % 128 != 0)
	{
		cellSpurs->Error("cellSpursInitialize : CELL_SPURS_CORE_ERROR_ALIGN");
		return CELL_SPURS_CORE_ERROR_ALIGN;
	}

	if (!spurs.IsGood())
	{
		cellSpurs->Error("cellSpursInitialize : CELL_SPURS_CORE_ERROR_NULL_POINTER");
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;
	}

	SPURSManagerAttribute *attr = new SPURSManagerAttribute(nSpus, spuPriority, ppuPriority, exitIfNoWork);
	spurs->spurs = new SPURSManager(attr);

	return CELL_OK;
}

int cellSpursFinalize(mem_ptr_t<CellSpurs> spurs)
{
	cellSpurs->Warning("cellSpursFinalize(spurs_addr=0x%x)", spurs.GetAddr());

	if (spurs.GetAddr() % 128 != 0)
	{
		cellSpurs->Error("cellSpursFinalize : CELL_SPURS_CORE_ERROR_ALIGN");
		return CELL_SPURS_CORE_ERROR_ALIGN;
	}

	if (!spurs.IsGood())
	{
		cellSpurs->Error("cellSpursFinalize : CELL_SPURS_CORE_ERROR_NULL_POINTER");
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;
	}

	spurs->spurs->Finalize();

	return CELL_OK;
}

int cellSpursInitializeWithAttribute(mem_ptr_t<CellSpurs> spurs, const mem_ptr_t<CellSpursAttribute> attr)
{
	cellSpurs->Warning("cellSpursInitializeWithAttribute(spurs_addr=0x%x, spurs_addr=0x%x)", spurs.GetAddr(), attr.GetAddr());

	if ((spurs.GetAddr() % 128 != 0) || (attr.GetAddr() % 8 != 0))
	{
		cellSpurs->Error("cellSpursInitializeWithAttribute : CELL_SPURS_CORE_ERROR_ALIGN");
		return CELL_SPURS_CORE_ERROR_ALIGN;
	}

	if (!spurs.IsGood() || !attr.IsGood())
	{
		cellSpurs->Error("cellSpursInitializeWithAttribute : CELL_SPURS_CORE_ERROR_NULL_POINTER");
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;
	}

	spurs->spurs = new SPURSManager(attr->attr);

	return CELL_OK;
}

int cellSpursInitializeWithAttribute2(mem_ptr_t<CellSpurs2> spurs, const mem_ptr_t<CellSpursAttribute> attr)
{
	cellSpurs->Warning("cellSpursInitializeWithAttribute2(spurs_addr=0x%x, spurs_addr=0x%x)", spurs.GetAddr(), attr.GetAddr());

	if ((spurs.GetAddr() % 128 != 0) || (attr.GetAddr() % 8 != 0))
	{
		cellSpurs->Error("cellSpursInitializeWithAttribute2 : CELL_SPURS_CORE_ERROR_ALIGN");
		return CELL_SPURS_CORE_ERROR_ALIGN;
	}

	if (!spurs.IsGood() || !attr.IsGood())
	{
		cellSpurs->Error("cellSpursInitializeWithAttribute2 : CELL_SPURS_CORE_ERROR_NULL_POINTER");
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;
	}

	spurs->spurs = new SPURSManager(attr->attr);

	return CELL_OK;
}

int _cellSpursAttributeInitialize(mem_ptr_t<CellSpursAttribute> attr, int nSpus, int spuPriority, int ppuPriority, bool exitIfNoWork)
{
	cellSpurs->Warning("_cellSpursAttributeInitialize(attr_addr=0x%x, nSpus=%u, spuPriority=%u, ppuPriority=%u, exitIfNoWork=%u)", attr.GetAddr(), nSpus, spuPriority, ppuPriority, exitIfNoWork);

	if (attr.GetAddr() % 8 != 0)
	{
		cellSpurs->Error("_cellSpursAttributeInitialize : CELL_SPURS_CORE_ERROR_ALIGN");
		return CELL_SPURS_CORE_ERROR_ALIGN;
	}

	if (!attr.IsGood())
	{
		cellSpurs->Error("_cellSpursAttributeInitialize : CELL_SPURS_CORE_ERROR_NULL_POINTER");
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;
	}

	attr->attr = new SPURSManagerAttribute(nSpus, spuPriority, ppuPriority, exitIfNoWork);

	return CELL_OK;
}

int cellSpursAttributeSetMemoryContainerForSpuThread(mem_ptr_t<CellSpursAttribute> attr, u32 container)
{
	cellSpurs->Warning("cellSpursAttributeSetMemoryContainerForSpuThread(attr_addr=0x%x, container=0x%x)", attr.GetAddr(), container);

	if (attr.GetAddr() % 8 != 0)
	{
		cellSpurs->Error("cellSpursAttributeSetMemoryContainerForSpuThread : CELL_SPURS_CORE_ERROR_ALIGN");
		return CELL_SPURS_CORE_ERROR_ALIGN;
	}

	if (!attr.IsGood())
	{
		cellSpurs->Error("cellSpursAttributeSetMemoryContainerForSpuThread : CELL_SPURS_CORE_ERROR_NULL_POINTER");
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;
	}

	attr->attr->_setMemoryContainerForSpuThread(container);

	return CELL_OK;
}

int cellSpursAttributeSetNamePrefix(mem_ptr_t<CellSpursAttribute> attr, const mem8_t prefix, u32 size)
{
	cellSpurs->Warning("cellSpursAttributeSetNamePrefix(attr_addr=0x%x, prefix_addr=0x%x, size=0x%x)", attr.GetAddr(), prefix.GetAddr(), size);

	if (attr.GetAddr() % 8 != 0)
	{
		cellSpurs->Error("cellSpursAttributeSetNamePrefix : CELL_SPURS_CORE_ERROR_ALIGN");
		return CELL_SPURS_CORE_ERROR_ALIGN;
	}

	if (!attr.IsGood() || !prefix.IsGood())
	{
		cellSpurs->Error("cellSpursAttributeSetNamePrefix : CELL_SPURS_CORE_ERROR_NULL_POINTER");
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;
	}

	if (size > CELL_SPURS_NAME_MAX_LENGTH)
	{
		cellSpurs->Error("cellSpursAttributeSetNamePrefix : CELL_SPURS_CORE_ERROR_INVAL");
		return CELL_SPURS_CORE_ERROR_INVAL;
	}

	attr->attr->_setNamePrefix(Memory.ReadString(prefix.GetAddr(), size).c_str(), size);

	return CELL_OK;
}

int cellSpursAttributeEnableSpuPrintfIfAvailable(mem_ptr_t<CellSpursAttribute> attr)
{
	cellSpurs->Error("cellSpursAttributeEnableSpuPrintfIfAvailable(attr_addr=0x%x)", attr.GetAddr());

	if (attr.GetAddr() % 8 != 0)
	{
		cellSpurs->Error("cellSpursAttributeEnableSpuPrintfIfAvailable : CELL_SPURS_CORE_ERROR_ALIGN");
		return CELL_SPURS_CORE_ERROR_ALIGN;
	}

	if (!attr.IsGood())
	{
		cellSpurs->Error("cellSpursAttributeEnableSpuPrintfIfAvailable : CELL_SPURS_CORE_ERROR_NULL_POINTER");
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;
	}

	return CELL_OK;
}

int cellSpursAttributeSetSpuThreadGroupType(mem_ptr_t<CellSpursAttribute> attr, int type)
{
	cellSpurs->Warning("cellSpursAttributeSetSpuThreadGroupType(attr_addr=0x%x, type=%u)", attr.GetAddr(), type);

	if (attr.GetAddr() % 8 != 0)
	{
		cellSpurs->Error("cellSpursAttributeSetNamePrefix : CELL_SPURS_CORE_ERROR_ALIGN");
		return CELL_SPURS_CORE_ERROR_ALIGN;
	}

	if (!attr.IsGood())
	{
		cellSpurs->Error("cellSpursAttributeSetSpuThreadGroupType : CELL_SPURS_CORE_ERROR_NULL_POINTER");
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;
	}

	attr->attr->_setSpuThreadGroupType(type);

	return CELL_OK;
}

int cellSpursAttributeEnableSystemWorkload(mem_ptr_t<CellSpursAttribute> attr, const u8 priority[CELL_SPURS_MAX_SPU],
	u32 maxSpu, const bool isPreemptible[CELL_SPURS_MAX_SPU])
{
	cellSpurs->Error("cellSpursAttributeEnableSystemWorkload(attr_addr=0x%x, priority[%u], maxSpu=%u, isPreemptible[%u])", attr.GetAddr(), priority, maxSpu, isPreemptible);

	if (attr.GetAddr() % 8 != 0)
	{
		cellSpurs->Error("cellSpursAttributeEnableSystemWorkload : CELL_SPURS_CORE_ERROR_ALIGN");
		return CELL_SPURS_CORE_ERROR_ALIGN;
	}

	if (!attr.IsGood())
	{
		cellSpurs->Error("cellSpursAttributeEnableSystemWorkload : CELL_SPURS_CORE_ERROR_NULL_POINTER");
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;
	}

	for (int i = 0; i < CELL_SPURS_MAX_SPU; i++)
	{
		if (priority[i] != 1 || maxSpu == 0)
		{
			cellSpurs->Error("cellSpursAttributeEnableSystemWorkload : CELL_SPURS_CORE_ERROR_INVAL");
			return CELL_SPURS_CORE_ERROR_INVAL;
		}
	}
	return CELL_OK;
}

int cellSpursGetSpuThreadGroupId(mem_ptr_t<CellSpurs> spurs, mem32_t group)
{
	cellSpurs->Error("cellSpursGetSpuThreadGroupId(spurs_addr=0x%x, group_addr=0x%x)", spurs.GetAddr(), group.GetAddr());

	if (spurs.GetAddr() % 128 != 0)
	{
		cellSpurs->Error("cellSpursGetSpuThreadGroupId : CELL_SPURS_CORE_ERROR_ALIGN");
		return CELL_SPURS_CORE_ERROR_ALIGN;
	}

	if (!spurs.IsGood() || group.IsGood())
	{
		cellSpurs->Error("cellSpursGetSpuThreadGroupId : CELL_SPURS_CORE_ERROR_NULL_POINTER");
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;
	}

	return CELL_OK;
}

int cellSpursGetNumSpuThread(mem_ptr_t<CellSpurs> spurs, mem32_t nThreads)
{
	cellSpurs->Error("cellSpursGetNumSpuThread(spurs_addr=0x%x, nThreads_addr=0x%x)", spurs.GetAddr(), nThreads.GetAddr());

	if (spurs.GetAddr() % 128 != 0)
	{
		cellSpurs->Error("cellSpursGetNumSpuThread : CELL_SPURS_CORE_ERROR_ALIGN");
		return CELL_SPURS_CORE_ERROR_ALIGN;
	}

	if (!spurs.IsGood() || nThreads.IsGood())
	{
		cellSpurs->Error("cellSpursGetNumSpuThread : CELL_SPURS_CORE_ERROR_NULL_POINTER");
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;
	}

	return CELL_OK;
}

int cellSpursGetSpuThreadId(mem_ptr_t<CellSpurs> spurs, mem32_t thread, mem32_t nThreads)
{
	cellSpurs->Error("cellSpursGetSpuThreadId(spurs_addr=0x%x, thread_addr=0x%x, nThreads_addr=0x%x)", spurs.GetAddr(), thread.GetAddr(), nThreads.GetAddr());

	if (spurs.GetAddr() % 128 != 0)
	{
		cellSpurs->Error("cellSpursGetSpuThreadId : CELL_SPURS_CORE_ERROR_ALIGN");
		return CELL_SPURS_CORE_ERROR_ALIGN;
	}

	if (!spurs.IsGood() || !thread.IsGood() || nThreads.IsGood())
	{
		cellSpurs->Error("cellSpursGetSpuThreadId : CELL_SPURS_CORE_ERROR_NULL_POINTER");
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;
	}

	return CELL_OK;
}

int cellSpursSetMaxContention(mem_ptr_t<CellSpurs> spurs, u32 workloadId, u32 maxContention)
{
	cellSpurs->Error("cellSpursSetMaxContention(spurs_addr=0x%x, workloadId=%u, maxContention=%u)", spurs.GetAddr(), workloadId, maxContention);

	if (spurs.GetAddr() % 128 != 0)
	{
		cellSpurs->Error("cellSpursSetMaxContention : CELL_SPURS_CORE_ERROR_ALIGN");
		return CELL_SPURS_CORE_ERROR_ALIGN;
	}

	if (!spurs.IsGood())
	{
		cellSpurs->Error("cellSpursSetMaxContention : CELL_SPURS_CORE_ERROR_NULL_POINTER");
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;
	}

	return CELL_OK;
}

int cellSpursSetPriorities(mem_ptr_t<CellSpurs> spurs, u32 workloadId, const u8 priorities[CELL_SPURS_MAX_SPU])
{
	cellSpurs->Error("cellSpursSetPriorities(spurs_addr=0x%x, workloadId=%u, priorities[%u])", spurs.GetAddr(), workloadId, priorities);

	if (spurs.GetAddr() % 128 != 0)
	{
		cellSpurs->Error("cellSpursSetPriorities : CELL_SPURS_CORE_ERROR_ALIGN");
		return CELL_SPURS_CORE_ERROR_ALIGN;
	}

	if (!spurs.IsGood())
	{
		cellSpurs->Error("cellSpursSetPriorities : CELL_SPURS_CORE_ERROR_NULL_POINTER");
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;
	}

	return CELL_OK;
}

int cellSpursSetPriority(mem_ptr_t<CellSpurs> spurs, u32 workloadId, u32 spuId, u32 priority)
{
	cellSpurs->Error("cellSpursSetPriority(spurs_addr=0x%x, workloadId=%u, spuId=%u, priority=%u)", spurs.GetAddr(), workloadId, spuId, priority);

	if (spurs.GetAddr() % 128 != 0)
	{
		cellSpurs->Error("cellSpursSetPriority : CELL_SPURS_CORE_ERROR_ALIGN");
		return CELL_SPURS_CORE_ERROR_ALIGN;
	}

	if (!spurs.IsGood())
	{
		cellSpurs->Error("cellSpursSetPriority : CELL_SPURS_CORE_ERROR_NULL_POINTER");
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;
	}

	return CELL_OK;
}

int cellSpursSetPreemptionVictimHints(mem_ptr_t<CellSpurs> spurs, const bool isPreemptible[CELL_SPURS_MAX_SPU])
{
	cellSpurs->Error("cellSpursSetPreemptionVictimHints(spurs_addr=0x%x, isPreemptible[%u])", spurs.GetAddr(), isPreemptible);

	if (spurs.GetAddr() % 128 != 0)
	{
		cellSpurs->Error("cellSpursSetPreemptionVictimHints : CELL_SPURS_CORE_ERROR_ALIGN");
		return CELL_SPURS_CORE_ERROR_ALIGN;
	}

	if (!spurs.IsGood())
	{
		cellSpurs->Error("cellSpursSetPreemptionVictimHints : CELL_SPURS_CORE_ERROR_NULL_POINTER");
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;
	}

	return CELL_OK;
}

int cellSpursAttachLv2EventQueue(mem_ptr_t<CellSpurs> spurs, u32 queue, mem8_t port, int isDynamic)
{
	cellSpurs->Warning("cellSpursAttachLv2EventQueue(spurs_addr=0x%x, queue=0x%x, port_addr=0x%x, isDynamic=%u)", spurs.GetAddr(), queue, port.GetAddr(), isDynamic);

	if (spurs.GetAddr() % 128 != 0)
	{
		cellSpurs->Error("cellSpursAttachLv2EventQueue : CELL_SPURS_CORE_ERROR_ALIGN");
		return CELL_SPURS_CORE_ERROR_ALIGN;
	}

	if (!spurs.IsGood() || !port.IsGood())
	{
		cellSpurs->Error("cellSpursAttachLv2EventQueue : CELL_SPURS_CORE_ERROR_NULL_POINTER");
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;
	}

	spurs->spurs->AttachLv2EventQueue(queue, port, isDynamic);

	return CELL_OK;
}

int cellSpursDetachLv2EventQueue(mem_ptr_t<CellSpurs> spurs, u8 port)
{
	cellSpurs->Warning("cellSpursDetachLv2EventQueue(spurs_addr=0x%x, port=0x%x)", spurs.GetAddr(), port);

	if (spurs.GetAddr() % 128 != 0)
	{
		cellSpurs->Error("cellSpursDetachLv2EventQueue : CELL_SPURS_CORE_ERROR_ALIGN");
		return CELL_SPURS_CORE_ERROR_ALIGN;
	}

	if (!spurs.IsGood())
	{
		cellSpurs->Error("cellSpursDetachLv2EventQueue : CELL_SPURS_CORE_ERROR_NULL_POINTER");
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;
	}

	spurs->spurs->DetachLv2EventQueue(port);

	return CELL_OK;
}

int cellSpursEnableExceptionEventHandler(mem_ptr_t<CellSpurs> spurs, bool flag)
{
	cellSpurs->Error("cellSpursEnableExceptionEventHandler(spurs_addr=0x%x, flag=%u)", spurs.GetAddr(), flag);

	if (spurs.GetAddr() % 128 != 0)
	{
		cellSpurs->Error("cellSpursEnableExceptionEventHandler : CELL_SPURS_CORE_ERROR_ALIGN");
		return CELL_SPURS_CORE_ERROR_ALIGN;
	}

	if (!spurs.IsGood())
	{
		cellSpurs->Error("cellSpursEnableExceptionEventHandler : CELL_SPURS_CORE_ERROR_NULL_POINTER");
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;
	}

	return CELL_OK;
}

int cellSpursSetGlobalExceptionEventHandler(mem_ptr_t<CellSpurs> spurs, mem_func_ptr_t<CellSpursGlobalExceptionEventHandler> eaHandler, mem_ptr_t<void> arg)
{
	cellSpurs->Error("cellSpursSetGlobalExceptionEventHandler(spurs_addr=0x%x, eaHandler_addr=0x%x, arg_addr=0x%x,)", spurs.GetAddr(), eaHandler.GetAddr(), arg.GetAddr());

	if (spurs.GetAddr() % 128 != 0)
	{
		cellSpurs->Error("cellSpursSetGlobalExceptionEventHandler : CELL_SPURS_CORE_ERROR_ALIGN");
		return CELL_SPURS_CORE_ERROR_ALIGN;
	}

	if (!spurs.IsGood() || eaHandler.IsGood())
	{
		cellSpurs->Error("cellSpursSetGlobalExceptionEventHandler : CELL_SPURS_CORE_ERROR_NULL_POINTER");
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;
	}

	return CELL_OK;
}

int cellSpursUnsetGlobalExceptionEventHandler(mem_ptr_t<CellSpurs> spurs)
{
	cellSpurs->Error("cellSpursUnsetGlobalExceptionEventHandler(spurs_addr=0x%x)", spurs.GetAddr());

	if (spurs.GetAddr() % 128 != 0)
	{
		cellSpurs->Error("cellSpursUnsetGlobalExceptionEventHandler : CELL_SPURS_CORE_ERROR_ALIGN");
		return CELL_SPURS_CORE_ERROR_ALIGN;
	}

	if (!spurs.IsGood())
	{
		cellSpurs->Error("cellSpursUnsetGlobalExceptionEventHandler : CELL_SPURS_CORE_ERROR_NULL_POINTER");
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;
	}

	return CELL_OK;
}

int cellSpursGetInfo(mem_ptr_t<CellSpurs> spurs, mem_ptr_t<CellSpursInfo> info)
{
	cellSpurs->Error("cellSpursGetInfo(spurs_addr=0x%x, info_addr=0x%x)", spurs.GetAddr(), info.GetAddr());

	if (spurs.GetAddr() % 128 != 0)
	{
		cellSpurs->Error("cellSpursGetInfo : CELL_SPURS_CORE_ERROR_ALIGN");
		return CELL_SPURS_CORE_ERROR_ALIGN;
	}

	if (!spurs.IsGood() || !info.IsGood())
	{
		cellSpurs->Error("cellSpursGetInfo : CELL_SPURS_CORE_ERROR_NULL_POINTER");
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;
	}

	return CELL_OK;
}

int _cellSpursEventFlagInitialize(mem_ptr_t<CellSpurs> spurs, mem_ptr_t<CellSpursTaskset> taskset, mem_ptr_t<CellSpursEventFlag> eventFlag, u32 flagClearMode, u32 flagDirection)
{
	cellSpurs->Warning("_cellSpursEventFlagInitialize(spurs_addr=0x%x, taskset_addr=0x%x, eventFlag_addr=0x%x, flagClearMode=%u, flagDirection=%u)", spurs.GetAddr(), taskset.GetAddr(), eventFlag.GetAddr(), flagClearMode, flagDirection);

	if ((taskset.GetAddr() % 128 != 0) || (eventFlag.GetAddr() % 128 != 0))
	{
		cellSpurs->Error("_cellSpursEventFlagInitialize : CELL_SPURS_TASK_ERROR_ALIGN");
		return CELL_SPURS_TASK_ERROR_ALIGN;
	}

	if ((!spurs.IsGood() && !taskset.IsGood()) || !eventFlag.IsGood())
	{
		cellSpurs->Error("_cellSpursEventFlagInitialize : CELL_SPURS_TASK_ERROR_NULL_POINTER");
		return CELL_SPURS_TASK_ERROR_NULL_POINTER;
	}

	eventFlag->eventFlag = new SPURSManagerEventFlag(flagClearMode, flagDirection);

	return CELL_OK;
}

int cellSpursEventFlagAttachLv2EventQueue(mem_ptr_t<CellSpursEventFlag> eventFlag)
{
	cellSpurs->Error("cellSpursEventFlagAttachLv2EventQueue(eventFlag_addr=0x%x)", eventFlag.GetAddr());

	if (eventFlag.GetAddr() % 128 != 0)
	{
		cellSpurs->Error("cellSpursEventFlagAttachLv2EventQueue : CELL_SPURS_TASK_ERROR_ALIGN");
		return CELL_SPURS_TASK_ERROR_ALIGN;
	}

	if (!eventFlag.IsGood())
	{
		cellSpurs->Error("cellSpursEventFlagAttachLv2EventQueue : CELL_SPURS_TASK_ERROR_NULL_POINTER");
		return CELL_SPURS_TASK_ERROR_NULL_POINTER;
	}

	return CELL_OK;
}

int cellSpursEventFlagDetachLv2EventQueue(mem_ptr_t<CellSpursEventFlag> eventFlag)
{
	cellSpurs->Error("cellSpursEventFlagDetachLv2EventQueue(eventFlag_addr=0x%x)", eventFlag.GetAddr());

	if (eventFlag.GetAddr() % 128 != 0)
	{
		cellSpurs->Error("cellSpursEventFlagDetachLv2EventQueue : CELL_SPURS_TASK_ERROR_ALIGN");
		return CELL_SPURS_TASK_ERROR_ALIGN;
	}

	if (!eventFlag.IsGood())
	{
		cellSpurs->Error("cellSpursEventFlagDetachLv2EventQueue : CELL_SPURS_TASK_ERROR_NULL_POINTER");
		return CELL_SPURS_TASK_ERROR_NULL_POINTER;
	}

	return CELL_OK;
}

int cellSpursEventFlagWait(mem_ptr_t<CellSpursEventFlag> eventFlag, mem16_t mask, u32 mode)
{
	cellSpurs->Error("cellSpursEventFlagWait(eventFlag_addr=0x%x, mask=0x%x, mode=%u)", eventFlag.GetAddr(), mask.GetAddr(), mode);

	if (eventFlag.GetAddr() % 128 != 0)
	{
		cellSpurs->Error("cellSpursEventFlagWait : CELL_SPURS_TASK_ERROR_ALIGN");
		return CELL_SPURS_TASK_ERROR_ALIGN;
	}

	if (!eventFlag.IsGood() || !mask.IsGood())
	{
		cellSpurs->Error("cellSpursEventFlagWait : CELL_SPURS_TASK_ERROR_NULL_POINTER");
		return CELL_SPURS_TASK_ERROR_NULL_POINTER;
	}

	return CELL_OK;
}

int cellSpursEventFlagClear(mem_ptr_t<CellSpursEventFlag> eventFlag, u16 bits)
{
	cellSpurs->Error("cellSpursEventFlagClear(eventFlag_addr=0x%x, bits=%u)", eventFlag.GetAddr(), bits);

	if (eventFlag.GetAddr() % 128 != 0)
	{
		cellSpurs->Error("cellSpursEventFlagClear : CELL_SPURS_TASK_ERROR_ALIGN");
		return CELL_SPURS_TASK_ERROR_ALIGN;
	}

	if (!eventFlag.IsGood())
	{
		cellSpurs->Error("cellSpursEventFlagClear : CELL_SPURS_TASK_ERROR_NULL_POINTER");
		return CELL_SPURS_TASK_ERROR_NULL_POINTER;
	}

	return CELL_OK;
}

int cellSpursEventFlagSet(mem_ptr_t<CellSpursEventFlag> eventFlag, u16 bits)
{
	cellSpurs->Error("cellSpursEventFlagSet(eventFlag_addr=0x%x, bits=%u)", eventFlag.GetAddr(), bits);

	if (eventFlag.GetAddr() % 128 != 0)
	{
		cellSpurs->Error("cellSpursEventFlagSet : CELL_SPURS_TASK_ERROR_ALIGN");
		return CELL_SPURS_TASK_ERROR_ALIGN;
	}

	if (!eventFlag.IsGood())
	{
		cellSpurs->Error("cellSpursEventFlagSet : CELL_SPURS_TASK_ERROR_NULL_POINTER");
		return CELL_SPURS_TASK_ERROR_NULL_POINTER;
	}

	return CELL_OK;
}

int cellSpursEventFlagTryWait(mem_ptr_t<CellSpursEventFlag> eventFlag, mem16_t mask, u32 mode)
{
	cellSpurs->Error("cellSpursEventFlagTryWait(eventFlag_addr=0x%x, mask_addr=0x%x, mode=%u)", eventFlag.GetAddr(), mask.GetAddr(), mode);

	if (eventFlag.GetAddr() % 128 != 0)
	{
		cellSpurs->Error("cellSpursEventFlagTryWait : CELL_SPURS_TASK_ERROR_ALIGN");
		return CELL_SPURS_TASK_ERROR_ALIGN;
	}

	if (!eventFlag.IsGood())
	{
		cellSpurs->Error("cellSpursEventFlagTryWait : CELL_SPURS_TASK_ERROR_NULL_POINTER");
		return CELL_SPURS_TASK_ERROR_NULL_POINTER;
	}

	return CELL_OK;
}

int cellSpursEventFlagGetDirection(mem_ptr_t<CellSpursEventFlag> eventFlag, mem32_t direction)
{
	cellSpurs->Warning("cellSpursEventFlagGetDirection(eventFlag_addr=0x%x, direction_addr=%u)", eventFlag.GetAddr(), direction.GetAddr());

	if (eventFlag.GetAddr() % 128 != 0)
	{
		cellSpurs->Error("cellSpursEventFlagGetDirection : CELL_SPURS_TASK_ERROR_ALIGN");
		return CELL_SPURS_TASK_ERROR_ALIGN;
	}

	if (!eventFlag.IsGood() || !direction.IsGood())
	{
		cellSpurs->Error("cellSpursEventFlagGetDirection : CELL_SPURS_TASK_ERROR_NULL_POINTER");
		return CELL_SPURS_TASK_ERROR_NULL_POINTER;
	}

	direction = eventFlag->eventFlag->_getDirection();

	return CELL_OK;
}

int cellSpursEventFlagGetClearMode(mem_ptr_t<CellSpursEventFlag> eventFlag, mem32_t clear_mode)
{
	cellSpurs->Warning("cellSpursEventFlagGetClearMode(eventFlag_addr=0x%x, clear_mode_addr=%u)", eventFlag.GetAddr(), clear_mode.GetAddr());

	if (eventFlag.GetAddr() % 128 != 0)
	{
		cellSpurs->Error("cellSpursEventFlagGetClearMode : CELL_SPURS_TASK_ERROR_ALIGN");
		return CELL_SPURS_TASK_ERROR_ALIGN;
	}

	if (!eventFlag.IsGood() || !clear_mode.IsGood())
	{
		cellSpurs->Error("cellSpursEventFlagGetClearMode : CELL_SPURS_TASK_ERROR_NULL_POINTER");
		return CELL_SPURS_TASK_ERROR_NULL_POINTER;
	}

	clear_mode = eventFlag->eventFlag->_getClearMode();

	return CELL_OK;
}

int cellSpursEventFlagGetTasksetAddress(mem_ptr_t<CellSpursEventFlag> eventFlag, mem_ptr_t<CellSpursTaskset> taskset)
{
	cellSpurs->Error("cellSpursEventFlagTryWait(eventFlag_addr=0x%x, taskset_addr=0x%x)", eventFlag.GetAddr(), taskset.GetAddr());

	if (eventFlag.GetAddr() % 128 != 0)
	{
		cellSpurs->Error("cellSpursEventFlagTryWait : CELL_SPURS_TASK_ERROR_ALIGN");
		return CELL_SPURS_TASK_ERROR_ALIGN;
	}

	if (!eventFlag.IsGood() || !taskset.IsGood())
	{
		cellSpurs->Error("cellSpursEventFlagTryWait : CELL_SPURS_TASK_ERROR_NULL_POINTER");
		return CELL_SPURS_TASK_ERROR_NULL_POINTER;
	}

	return CELL_OK;
}

int _cellSpursLFQueueInitialize()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

int _cellSpursLFQueuePushBody()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

int cellSpursLFQueueDetachLv2EventQueue()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

int cellSpursLFQueueAttachLv2EventQueue()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

int cellSpursQueueDetachLv2EventQueue()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

int _cellSpursLFQueuePopBody()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

int cellSpursLFQueueGetTasksetAddress()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

int cellSyncLFQueueGetEntrySize()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}
int cellSyncLFQueueSize()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

int cellSyncLFQueueClear()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}
int _cellSyncLFQueueCompletePushPointer2()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

int _cellSyncLFQueueGetPopPointer2()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

int cellSyncQueuePeek()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

int cellSyncQueueSize()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

int cellSyncQueuePop()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

int _cellSyncLFQueueCompletePushPointer()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

int _cellSyncLFQueueAttachLv2EventQueue()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

int cellSyncQueuePush()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

int cellSyncQueueTryPeek()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

int _cellSyncLFQueueGetPushPointer2()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

int cellSyncQueueTryPush()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

int _cellSyncLFQueueGetPopPointer()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

int _cellSyncLFQueueCompletePopPointer2()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

int _cellSyncLFQueueDetachLv2EventQueue()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

int cellSyncQueueClear()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

int cellSyncQueueTryPop()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

int cellSyncLFQueueInitialize()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

int _cellSyncLFQueueGetSignalAddress()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

int _cellSyncLFQueuePushBody()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

int cellSyncLFQueueGetDirection()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

int cellSyncLFQueueDepth()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

int _cellSyncLFQueuePopBody()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

int _cellSyncLFQueueGetPushPointer()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

int _cellSyncLFQueueCompletePopPointer()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

int _cellSpursQueueInitialize()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

int cellSpursQueuePopBody()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

int cellSpursQueuePushBody()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

int cellSpursQueueAttachLv2EventQueue()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

int cellSpursCreateJobChainWithAttribute()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

int cellSpursCreateJobChain()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

int cellSpursJoinJobChain()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

int cellSpursKickJobChain()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

int _cellSpursJobChainAttributeInitialize()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

int cellSpursCreateTasksetWithAttribute()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

int cellSpursCreateTaskset(mem_ptr_t<CellSpurs> spurs, mem_ptr_t<CellSpursTaskset> taskset, u64 args, mem8_t priority, u32 maxContention)
{
	cellSpurs->Error("cellSpursCreateTaskset(spurs_addr=0x%x, taskset_addr=0x%x, args=0x%x, priority_addr=0x%x, maxContention=%u)", spurs.GetAddr(), taskset.GetAddr(), args, priority.GetAddr(), maxContention);

	if ((spurs.GetAddr() % 128 != 0) || (taskset.GetAddr() % 128 != 0))
	{
		cellSpurs->Error("cellSpursCreateTaskset : CELL_SPURS_TASK_ERROR_ALIGN");
		return CELL_SPURS_TASK_ERROR_ALIGN;
	}

	if (!spurs.IsGood() || !taskset.IsGood())
	{
		cellSpurs->Error("cellSpursCreateTaskset : CELL_SPURS_TASK_ERROR_NULL_POINTER");
		return CELL_SPURS_TASK_ERROR_NULL_POINTER;
	}

	SPURSManagerTasksetAttribute *tattr = new SPURSManagerTasksetAttribute(args, priority, maxContention);
	taskset->taskset = new SPURSManagerTaskset(taskset.GetAddr(), tattr);

	return CELL_OK;
}

int cellSpursJoinTaskset(mem_ptr_t<CellSpursTaskset> taskset)
{
	cellSpurs->Error("cellSpursJoinTaskset(taskset_addr=0x%x)", taskset.GetAddr());

	if (taskset.GetAddr() % 128 != 0)
	{
		cellSpurs->Error("cellSpursJoinTaskset : CELL_SPURS_TASK_ERROR_ALIGN");
		return CELL_SPURS_TASK_ERROR_ALIGN;
	}

	if (!taskset.IsGood())
	{
		cellSpurs->Error("cellSpursJoinTaskset : CELL_SPURS_TASK_ERROR_NULL_POINTER");
		return CELL_SPURS_TASK_ERROR_NULL_POINTER;
	}

	return CELL_OK;
}

int cellSpursGetTasksetId(mem_ptr_t<CellSpursTaskset> taskset, mem32_t workloadId)
{
	cellSpurs->Error("cellSpursGetTasksetId(taskset_addr=0x%x, workloadId_addr=0x%x)", taskset.GetAddr(), workloadId.GetAddr());

	if (taskset.GetAddr() % 128 != 0)
	{
		cellSpurs->Error("cellSpursGetTasksetId : CELL_SPURS_TASK_ERROR_ALIGN");
		return CELL_SPURS_TASK_ERROR_ALIGN;
	}

	if (!taskset.IsGood() || !workloadId.IsGood())
	{
		cellSpurs->Error("cellSpursGetTasksetId : CELL_SPURS_TASK_ERROR_NULL_POINTER");
		return CELL_SPURS_TASK_ERROR_NULL_POINTER;
	}

	return CELL_OK;
}

int cellSpursShutdownTaskset(mem_ptr_t<CellSpursTaskset> taskset)
{
	cellSpurs->Error("cellSpursShutdownTaskset(taskset_addr=0x%x)", taskset.GetAddr());

	if (taskset.GetAddr() % 128 != 0)
	{
		cellSpurs->Error("cellSpursShutdownTaskset : CELL_SPURS_TASK_ERROR_ALIGN");
		return CELL_SPURS_TASK_ERROR_ALIGN;
	}

	if (!taskset.IsGood())
	{
		cellSpurs->Error("cellSpursShutdownTaskset : CELL_SPURS_TASK_ERROR_NULL_POINTER");
		return CELL_SPURS_TASK_ERROR_NULL_POINTER;
	}

	return CELL_OK;
}

int cellSpursCreateTask(mem_ptr_t<CellSpursTaskset> taskset, mem32_t taskID, mem_ptr_t<void> elf_addr,
	mem_ptr_t<void> context_addr, u32 context_size, mem_ptr_t<CellSpursTaskLsPattern> lsPattern,
	mem_ptr_t<CellSpursTaskArgument> argument)
{
	cellSpurs->Error("cellSpursCreateTask(taskset_addr=0x%x, taskID_addr=0x%x, elf_addr_addr=0x%x, context_addr_addr=0x%x, context_size=%u, lsPattern_addr=0x%x, argument_addr=0x%x)",
		taskset.GetAddr(), taskID.GetAddr(), elf_addr.GetAddr(), context_addr.GetAddr(), context_size, lsPattern.GetAddr(), argument.GetAddr());

	if (taskset.GetAddr() % 128 != 0)
	{
		cellSpurs->Error("cellSpursCreateTask : CELL_SPURS_TASK_ERROR_ALIGN");
		return CELL_SPURS_TASK_ERROR_ALIGN;
	}

	if (!taskset.IsGood())
	{
		cellSpurs->Error("cellSpursCreateTask : CELL_SPURS_TASK_ERROR_NULL_POINTER");
		return CELL_SPURS_TASK_ERROR_NULL_POINTER;
	}

	return CELL_OK;
}

int _cellSpursSendSignal(mem_ptr_t<CellSpursTaskset> taskset, u32 taskID)
{
	cellSpurs->Error("_cellSpursSendSignal(taskset_addr=0x%x, taskID=%u)", taskset.GetAddr(), taskID);

	if (taskset.GetAddr() % 128 != 0)
	{
		cellSpurs->Error("_cellSpursSendSignal : CELL_SPURS_TASK_ERROR_ALIGN");
		return CELL_SPURS_TASK_ERROR_ALIGN;
	}

	if (!taskset.IsGood())
	{
		cellSpurs->Error("_cellSpursSendSignal : CELL_SPURS_TASK_ERROR_NULL_POINTER");
		return CELL_SPURS_TASK_ERROR_NULL_POINTER;
	}

	return CELL_OK;
}

int cellSpursCreateTaskWithAttribute()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

int cellSpursTasksetAttributeSetName()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

int _cellSpursTasksetAttribute2Initialize()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

int cellSpursTaskExitCodeGet()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

int cellSpursTaskExitCodeInitialize()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

int cellSpursTaskExitCodeTryGet()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

int cellSpursTaskGetLoadableSegmentPattern()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

int cellSpursTaskGetReadOnlyAreaPattern()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

int cellSpursTaskGenerateLsPattern()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

int cellSpursTaskAttributeInitialize()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

int cellSpursTaskAttributeSetExitCodeContainer()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

int _cellSpursTaskAttribute2Initialize()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

int cellSpursTaskGetContextSaveAreaSize()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

int cellSpursCreateTaskset2()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

int cellSpursCreateTask2()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

int cellSpursJoinTask2()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

int cellSpursTryJoinTask2()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

int cellSpursDestroyTaskset2()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

int cellSpursCreateTask2WithBinInfo()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

int cellSpursLookUpTasksetAddress()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

int cellSpursTasksetGetSpursAddress()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

int cellSpurssetExceptionEventHandler()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

int cellSpursUnsetExceptionEventHandler()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

int cellSpursGetTasksetInfo()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

int _cellSpursTasksetAttributeInitialize()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

int cellSpursJobGuardInitialize()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

int cellSpursJobChainAttributeSetName()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

int cellSpursShutdownJobChain()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

int cellSpursJobChainAttributeSetHaltOnError()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

int cellSpursJobChainAttributesetJobTypeMemoryCheck()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

int cellSpursJobGuardNotify()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

int  cellSpursJobGuardReset()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

int cellSpursRunJobChain()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

int cellSpursJobChainGetError()
{
	UNIMPLEMENTED_FUNC(cellSpurs);
	return CELL_OK;
}

void cellSpurs_init()
{
	// Core 
	cellSpurs->AddFunc(0xacfc8dbc, cellSpursInitialize);
	cellSpurs->AddFunc(0xaa6269a8, cellSpursInitializeWithAttribute);
	cellSpurs->AddFunc(0x30aa96c4, cellSpursInitializeWithAttribute2);
	cellSpurs->AddFunc(0xca4c4600, cellSpursFinalize);
	cellSpurs->AddFunc(0x95180230, _cellSpursAttributeInitialize);
	cellSpurs->AddFunc(0x82275c1c, cellSpursAttributeSetMemoryContainerForSpuThread);
	cellSpurs->AddFunc(0x07529113, cellSpursAttributeSetNamePrefix);
	cellSpurs->AddFunc(0x1051d134, cellSpursAttributeEnableSpuPrintfIfAvailable);
	cellSpurs->AddFunc(0xa839a4d9, cellSpursAttributeSetSpuThreadGroupType);
	cellSpurs->AddFunc(0x9dcbcb5d, cellSpursAttributeEnableSystemWorkload);
	cellSpurs->AddFunc(0x39c173fb, cellSpursGetSpuThreadGroupId);
	cellSpurs->AddFunc(0xc56defb5, cellSpursGetNumSpuThread);
	cellSpurs->AddFunc(0x6c960f6d, cellSpursGetSpuThreadId);
	cellSpurs->AddFunc(0x1f402f8f, cellSpursGetInfo);
	cellSpurs->AddFunc(0x84d2f6d5, cellSpursSetMaxContention);
	cellSpurs->AddFunc(0x80a29e27, cellSpursSetPriorities);
	// cellSpurs->AddFunc(<id>, cellSpursSetPriority);
	cellSpurs->AddFunc(0x4de203e2, cellSpursSetPreemptionVictimHints);
	cellSpurs->AddFunc(0xb9bc6207, cellSpursAttachLv2EventQueue);
	cellSpurs->AddFunc(0x4e66d483, cellSpursDetachLv2EventQueue);
	cellSpurs->AddFunc(0x32b94add, cellSpursEnableExceptionEventHandler);
	cellSpurs->AddFunc(0x7517724a, cellSpursSetGlobalExceptionEventHandler);
	cellSpurs->AddFunc(0x861237f8, cellSpursUnsetGlobalExceptionEventHandler);

	// Event flag
	cellSpurs->AddFunc(0x5ef96465, _cellSpursEventFlagInitialize);
	cellSpurs->AddFunc(0x87630976, cellSpursEventFlagAttachLv2EventQueue);
	cellSpurs->AddFunc(0x22aab31d, cellSpursEventFlagDetachLv2EventQueue);
	cellSpurs->AddFunc(0x373523d4, cellSpursEventFlagWait);
	cellSpurs->AddFunc(0x4ac7bae4, cellSpursEventFlagClear);
	cellSpurs->AddFunc(0xf5507729, cellSpursEventFlagSet);
	cellSpurs->AddFunc(0x6d2d9339, cellSpursEventFlagTryWait);
	cellSpurs->AddFunc(0x890f9e5a, cellSpursEventFlagGetDirection);
	cellSpurs->AddFunc(0x4d1e9373, cellSpursEventFlagGetClearMode);
	cellSpurs->AddFunc(0x947efb0b, cellSpursEventFlagGetTasksetAddress);

	// Taskset
	cellSpurs->AddFunc(0x52cc6c82, cellSpursCreateTaskset);
	cellSpurs->AddFunc(0xc10931cb, cellSpursCreateTasksetWithAttribute);
	cellSpurs->AddFunc(0x16394a4e, _cellSpursTasksetAttributeInitialize);
	cellSpurs->AddFunc(0xc2acdf43, _cellSpursTasksetAttribute2Initialize);
	cellSpurs->AddFunc(0x652b70e2, cellSpursTasksetAttributeSetName);
	cellSpurs->AddFunc(0x9f72add3, cellSpursJoinTaskset);
	cellSpurs->AddFunc(0xe7dd87e1, cellSpursGetTasksetId);
	cellSpurs->AddFunc(0xa789e631, cellSpursShutdownTaskset);
	cellSpurs->AddFunc(0xbeb600ac, cellSpursCreateTask);
	cellSpurs->AddFunc(0x1d46fedf, cellSpursCreateTaskWithAttribute);
	cellSpurs->AddFunc(0xb8474eff, cellSpursTaskAttributeInitialize);
	cellSpurs->AddFunc(0x8adadf65, _cellSpursTaskAttribute2Initialize);
	cellSpurs->AddFunc(0xa121a224, cellSpursTaskAttributeSetExitCodeContainer);
	cellSpurs->AddFunc(0x13ae18f3, cellSpursTaskExitCodeGet);
	cellSpurs->AddFunc(0x34552fa6, cellSpursTaskExitCodeInitialize);
	cellSpurs->AddFunc(0xe717ac73, cellSpursTaskExitCodeTryGet);
	cellSpurs->AddFunc(0x1d344406, cellSpursTaskGetLoadableSegmentPattern);
	cellSpurs->AddFunc(0x7cb33c2e, cellSpursTaskGetReadOnlyAreaPattern);
	cellSpurs->AddFunc(0x9197915f, cellSpursTaskGenerateLsPattern);
	cellSpurs->AddFunc(0x9034e538, cellSpursTaskGetContextSaveAreaSize);
	cellSpurs->AddFunc(0xe0a6dbe4, _cellSpursSendSignal);
	cellSpurs->AddFunc(0x4a6465e3, cellSpursCreateTaskset2);
	cellSpurs->AddFunc(0xe14ca62d, cellSpursCreateTask2);
	cellSpurs->AddFunc(0xa7a94892, cellSpursJoinTask2);
	cellSpurs->AddFunc(0x838fa4f0, cellSpursTryJoinTask2);
	cellSpurs->AddFunc(0x1ebcf459, cellSpursDestroyTaskset2);
	cellSpurs->AddFunc(0xe4944a1c, cellSpursCreateTask2WithBinInfo);
	cellSpurs->AddFunc(0x4cce88a9, cellSpursLookUpTasksetAddress);
	cellSpurs->AddFunc(0x58d58fcf, cellSpursTasksetGetSpursAddress);
	cellSpurs->AddFunc(0xd2e23fa9, cellSpurssetExceptionEventHandler);
	cellSpurs->AddFunc(0x4c75deb8, cellSpursUnsetExceptionEventHandler);
	cellSpurs->AddFunc(0x9fcb567b, cellSpursGetTasksetInfo);

	// Job Chain
	cellSpurs->AddFunc(0x60eb2dec, cellSpursCreateJobChain);
	cellSpurs->AddFunc(0x303c19cd, cellSpursCreateJobChainWithAttribute);
	cellSpurs->AddFunc(0x738e40e6, cellSpursShutdownJobChain);
	cellSpurs->AddFunc(0xa7c066de, cellSpursJoinJobChain);
	cellSpurs->AddFunc(0xbfea60fa, cellSpursKickJobChain);
	cellSpurs->AddFunc(0xf31731bb, cellSpursRunJobChain);
	cellSpurs->AddFunc(0x161da6a7, cellSpursJobChainGetError);
	cellSpurs->AddFunc(0x3548f483, _cellSpursJobChainAttributeInitialize);
	cellSpurs->AddFunc(0x9fef70c2, cellSpursJobChainAttributeSetName);
	cellSpurs->AddFunc(0xbb68d76e, cellSpursJobChainAttributeSetHaltOnError);
	cellSpurs->AddFunc(0x2cfccb99, cellSpursJobChainAttributesetJobTypeMemoryCheck);

	// Job Guard
	cellSpurs->AddFunc(0x68aaeba9, cellSpursJobGuardInitialize);
	cellSpurs->AddFunc(0xd5d0b256, cellSpursJobGuardNotify);
	cellSpurs->AddFunc(0x00af2519, cellSpursJobGuardReset);
	
	// Queue/LFQueue
	cellSpurs->AddFunc(0x011ee38b, _cellSpursLFQueueInitialize);
	cellSpurs->AddFunc(0x8a85674d, _cellSpursLFQueuePushBody);
	cellSpurs->AddFunc(0x1656d49f, cellSpursLFQueueAttachLv2EventQueue);
	cellSpurs->AddFunc(0x73e06f91, cellSpursLFQueueDetachLv2EventQueue);
	cellSpurs->AddFunc(0x082bfb09, _cellSpursQueueInitialize);
	cellSpurs->AddFunc(0x91066667, cellSpursQueuePopBody);
	cellSpurs->AddFunc(0x92cff6ed, cellSpursQueuePushBody);
	cellSpurs->AddFunc(0xe5443be7, cellSpursQueueAttachLv2EventQueue);
	cellSpurs->AddFunc(0x039d70b7, cellSpursQueueDetachLv2EventQueue);
	cellSpurs->AddFunc(0x35dae22b, _cellSpursLFQueuePopBody);
	cellSpurs->AddFunc(0xb792ca1a, cellSpursLFQueueGetTasksetAddress);
	cellSpurs->AddFunc(0x0c7cb9f7, cellSyncLFQueueGetEntrySize);
	cellSpurs->AddFunc(0x167ea63e, cellSyncLFQueueSize);
	cellSpurs->AddFunc(0x2af0c515, cellSyncLFQueueClear);
	cellSpurs->AddFunc(0x35bbdad2, _cellSyncLFQueueCompletePushPointer2);
	cellSpurs->AddFunc(0x46356fe0, _cellSyncLFQueueGetPopPointer2);
	cellSpurs->AddFunc(0x48154c9b, cellSyncQueuePeek);
	cellSpurs->AddFunc(0x4da349b2, cellSyncQueueSize);
	cellSpurs->AddFunc(0x4da6d7e0, cellSyncQueuePop);
	cellSpurs->AddFunc(0x4e88c68d, _cellSyncLFQueueCompletePushPointer);
	cellSpurs->AddFunc(0x54fc2032, _cellSyncLFQueueAttachLv2EventQueue);
	cellSpurs->AddFunc(0x5ae841e5, cellSyncQueuePush);
	cellSpurs->AddFunc(0x68af923c, cellSyncQueueTryPeek);
	cellSpurs->AddFunc(0x6bb4ef9d,_cellSyncLFQueueGetPushPointer2);
	cellSpurs->AddFunc(0x705985cd, cellSyncQueueTryPush);
	cellSpurs->AddFunc(0x74c37666, _cellSyncLFQueueGetPopPointer);
	cellSpurs->AddFunc(0x7a51deee, _cellSyncLFQueueCompletePopPointer2);
	cellSpurs->AddFunc(0x811d148e, _cellSyncLFQueueDetachLv2EventQueue);
	cellSpurs->AddFunc(0xa5362e73, cellSyncQueueClear);
	cellSpurs->AddFunc(0xa58df87f, cellSyncQueueTryPop);
	cellSpurs->AddFunc(0xaa355278, cellSyncLFQueueInitialize);
	cellSpurs->AddFunc(0xaff7627a, _cellSyncLFQueueGetSignalAddress);
	cellSpurs->AddFunc(0xba5961ca, _cellSyncLFQueuePushBody);
	cellSpurs->AddFunc(0xd59aa307, cellSyncLFQueueGetDirection);
	cellSpurs->AddFunc(0xe18c273c, cellSyncLFQueueDepth);
	cellSpurs->AddFunc(0xe1bc7add, _cellSyncLFQueuePopBody);
	cellSpurs->AddFunc(0xe9bf2110, _cellSyncLFQueueGetPushPointer);
	cellSpurs->AddFunc(0xfe74e8e7, _cellSyncLFQueueCompletePopPointer);

}

