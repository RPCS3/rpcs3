#include "stdafx.h"
#include "cellSpurs.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/SC_FUNC.h"

void cellSpurs_init();
Module cellSpurs(0x000a, cellSpurs_init);

int cellSpursInitialize(mem_ptr_t<CellSpurs> spurs, int nSpus, int spuPriority, 
									int ppuPriority, bool exitIfNoWork)
{
	cellSpurs.Warning("cellSpursInitialize(spurs_addr=0x%x, nSpus=%u, spuPriority=%u, ppuPriority=%u, exitIfNoWork=%u)",
		spurs.GetAddr(), nSpus, spuPriority, ppuPriority, exitIfNoWork);

	if(!spurs.IsGood())
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;

	SPURSManagerAttribute *attr = new SPURSManagerAttribute(nSpus, spuPriority, ppuPriority, exitIfNoWork);
	spurs->spurs = new SPURSManager(attr);

	return CELL_OK;
}

int cellSpursFinalize(mem_ptr_t<CellSpurs> spurs)
{
	cellSpurs.Warning("cellSpursFinalize(spurs_addr=0x%x)", spurs.GetAddr());

	if(!spurs.IsGood())
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;

	spurs->spurs->Finalize();

	return CELL_OK;
}

int cellSpursInitializeWithAttribute(mem_ptr_t<CellSpurs> spurs, const mem_ptr_t<CellSpursAttribute> attr)
{
	cellSpurs.Warning("cellSpursInitializeWithAttribute(spurs_addr=0x%x, spurs_addr=0x%x)",
						spurs.GetAddr(), attr.GetAddr());
	
	if(!attr.IsGood() || !spurs.IsGood())
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;

	spurs->spurs = new SPURSManager(attr->attr);

	return CELL_OK;
}

int cellSpursInitializeWithAttribute2(mem_ptr_t<CellSpurs2> spurs, const mem_ptr_t<CellSpursAttribute> attr)
{
	cellSpurs.Warning("cellSpursInitializeWithAttribute2(spurs_addr=0x%x, spurs_addr=0x%x)",
						spurs.GetAddr(), attr.GetAddr());
	
	if(!attr.IsGood() || !spurs.IsGood())
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;

	spurs->spurs = new SPURSManager(attr->attr);

	return CELL_OK;
}

int _cellSpursAttributeInitialize(mem_ptr_t<CellSpursAttribute> attr, int nSpus, int spuPriority, 
									int ppuPriority, bool exitIfNoWork)
{
	cellSpurs.Warning("_cellSpursAttributeInitialize(attr_addr=0x%x, nSpus=%u, spuPriority=%u, ppuPriority=%u, exitIfNoWork=%u)",
						attr.GetAddr(), nSpus, spuPriority, ppuPriority, exitIfNoWork);
	
	if(!attr.IsGood())
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;

	attr->attr = new SPURSManagerAttribute(nSpus, spuPriority, ppuPriority, exitIfNoWork);

	return CELL_OK;
}

int cellSpursAttributeSetMemoryContainerForSpuThread(mem_ptr_t<CellSpursAttribute> attr, u32 container)
{
	cellSpurs.Error("cellSpursAttributeSetMemoryContainerForSpuThread(attr_addr=0x%x, container=0x%x)",
						attr.GetAddr(), container);

	if(!attr.IsGood())
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;

	return CELL_OK;
}

int cellSpursAttributeSetNamePrefix(mem_ptr_t<CellSpursAttribute> attr, const mem8_t prefix, u32 size)
{
	cellSpurs.Warning("cellSpursAttributeSetNamePrefix(attr_addr=0x%x, prefix_addr=0x%x, size=0x%x)",
						attr.GetAddr(), prefix.GetAddr(), size);
	
	if(!attr.IsGood() || !prefix.IsGood())
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;
	
	if(size > 15)
		return CELL_SPURS_CORE_ERROR_INVAL;

	attr->attr->_setNamePrefix(Memory.ReadString(prefix.GetAddr(), size).c_str(), size);

	return CELL_OK;
}

int cellSpursAttributeEnableSpuPrintfIfAvailable(mem_ptr_t<CellSpursAttribute> attr)
{
	cellSpurs.Error("cellSpursAttributeEnableSpuPrintfIfAvailable(attr_addr=0x%x)", attr.GetAddr());
	
	if(!attr.IsGood())
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;

	return CELL_OK;
}

int cellSpursAttributeSetSpuThreadGroupType(mem_ptr_t<CellSpursAttribute> attr, int type)
{
	cellSpurs.Warning("cellSpursAttributeSetSpuThreadGroupType(attr_addr=0x%x, type=%u)", attr.GetAddr(), type);
	
	if(!attr.IsGood())
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;

	attr->attr->_setSpuThreadGroupType(type);

	return CELL_OK;
}

int cellSpursAttributeEnableSystemWorkload(mem_ptr_t<CellSpursAttribute> attr, const u8 priority[8], 
										   u32 maxSpu, const bool isPreemptible[8])
{
	cellSpurs.Error("cellSpursAttributeEnableSystemWorkload(attr_addr=0x%x, priority[%u], maxSpu=%u, isPreemptible[%u])",
						attr.GetAddr(), priority, maxSpu, isPreemptible);
	
	if(!attr.IsGood())
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;
	
	for (int i=0; i<8; i++)
		if(priority[i] != 1 || maxSpu == 0)
			return CELL_SPURS_CORE_ERROR_INVAL;
 
	return CELL_OK;
}

int cellSpursGetSpuThreadGroupId(mem_ptr_t<CellSpurs> spurs, mem32_t group)
{
	cellSpurs.Error("cellSpursGetSpuThreadGroupId(spurs_addr=0x%x, group_addr=0x%x)", 
		spurs.GetAddr(), group.GetAddr());

	if(!spurs.IsGood() || group.IsGood())
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;

	return CELL_OK;
}

int cellSpursGetNumSpuThread(mem_ptr_t<CellSpurs> spurs, mem32_t nThreads)
{
	cellSpurs.Error("cellSpursGetNumSpuThread(spurs_addr=0x%x, nThreads_addr=0x%x)", 
		spurs.GetAddr(), nThreads.GetAddr());
	
	if(!spurs.IsGood() || nThreads.IsGood())
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;

	return CELL_OK;
}

int cellSpursGetSpuThreadId(mem_ptr_t<CellSpurs> spurs, mem32_t thread, mem32_t nThreads)
{
	cellSpurs.Error("cellSpursGetSpuThreadId(spurs_addr=0x%x, thread_addr=0x%x, nThreads_addr=0x%x)", 
		spurs.GetAddr(), thread.GetAddr(), nThreads.GetAddr());
	
	if(!spurs.IsGood() || !thread.IsGood() || nThreads.IsGood())
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;

	return CELL_OK;
}

int cellSpursSetMaxContention(mem_ptr_t<CellSpurs> spurs, u32 workloadId, u32 maxContention)
{
	cellSpurs.Error("cellSpursSetMaxContention(spurs_addr=0x%x, workloadId=%u, maxContention=%u)", 
		spurs.GetAddr(), workloadId, maxContention);
	
	if(!spurs.IsGood())
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;

	return CELL_OK;
}

int cellSpursSetPriorities(mem_ptr_t<CellSpurs> spurs, u32 workloadId, const u8 priorities[CELL_SPURS_MAX_SPU])
{
	cellSpurs.Error("cellSpursSetPriorities(spurs_addr=0x%x, workloadId=%u, priorities[%u])", 
		spurs.GetAddr(), workloadId, priorities);
	
	if(!spurs.IsGood())
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;

	return CELL_OK;
}

int cellSpursSetPriority(mem_ptr_t<CellSpurs> spurs, u32 workloadId, u32 spuId, u32 priority)
{
	cellSpurs.Error("cellSpursSetPriority(spurs_addr=0x%x, workloadId=%u, spuId=%u, priority=%u)", 
		spurs.GetAddr(), workloadId, spuId, priority);
	
	if(!spurs.IsGood())
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;

	return CELL_OK;
}

int cellSpursSetPreemptionVictimHints(mem_ptr_t<CellSpurs> spurs, const bool isPreemptible[8])
{
	cellSpurs.Error("cellSpursSetPreemptionVictimHints(spurs_addr=0x%x, isPreemptible[%u])", 
						spurs.GetAddr(), isPreemptible);
	
	if(!spurs.IsGood())
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;

	return CELL_OK;
}

int cellSpursAttachLv2EventQueue(mem_ptr_t<CellSpurs> spurs, u32 queue, mem8_t port, int isDynamic)
{
	cellSpurs.Error("cellSpursAttachLv2EventQueue(spurs_addr=0x%x, queue=0x%x, port_addr=0x%x, isDynamic=%u)", 
						spurs.GetAddr(), queue, port.GetAddr(), isDynamic);
	
	if(!spurs.IsGood() || !port.IsGood())
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;

	return CELL_OK;
}

int cellSpursDetachLv2EventQueue(mem_ptr_t<CellSpurs> spurs, u8 port)
{
	cellSpurs.Error("cellSpursDetachLv2EventQueue(spurs_addr=0x%x, port=0x%x)", spurs.GetAddr(), port);
	
	if(!spurs.IsGood())
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;

	return CELL_OK;
}

int _cellSpursEventFlagInitialize(mem_ptr_t<CellSpurs> spurs, mem_ptr_t<CellSpursTaskset> taskset,
								  mem_ptr_t<CellSpursEventFlag> eventFlag, u32 flagClearMode, u32 flagDirection)
{
	cellSpurs.Error("_cellSpursEventFlagInitialize(spurs_addr=0x%x, taskset_addr=0x%x, eventFlag_addr=0x%x, flagClearMode=%u, flagDirection=%u)", 
		spurs.GetAddr(), taskset.GetAddr(), eventFlag.GetAddr(), flagClearMode, flagDirection);
	
	if(!spurs.IsGood() || !taskset.IsGood() || !eventFlag.IsGood())
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;

	return CELL_OK;
}

int cellSpursEventFlagAttachLv2EventQueue(mem_ptr_t<CellSpursEventFlag> event_flag)
{
	cellSpurs.Error("cellSpursEventFlagAttachLv2EventQueue(event_flag_addr=0x%x)", event_flag.GetAddr());
	
	if(!event_flag.IsGood())
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;

	return CELL_OK;
}

int cellSpursEventFlagDetachLv2EventQueue(mem_ptr_t<CellSpursEventFlag> event_flag)
{
	cellSpurs.Error("cellSpursEventFlagDetachLv2EventQueue(event_flag_addr=0x%x)", event_flag.GetAddr());
	
	if(!event_flag.IsGood())
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;

	return CELL_OK;
}

int cellSpursEventFlagWait(mem_ptr_t<CellSpursEventFlag> event_flag, mem16_t flag_bits, u32 wait_mode)
{
	cellSpurs.Error("cellSpursEventFlagWait(event_flag_addr=0x%x, flag_bits_addr=0x%x, wait_mode=%u)", 
		event_flag.GetAddr(), flag_bits.GetAddr(), wait_mode);
	
	if(!event_flag.IsGood() || !flag_bits.IsGood())
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;

	return CELL_OK;
}

int cellSpursEnableExceptionEventHandler(mem_ptr_t<CellSpurs> spurs, bool flag)
{
	cellSpurs.Error("cellSpursEnableExceptionEventHandler(spurs_addr=0x%x, flag=%u)", spurs.GetAddr(), flag);
	
	if(!spurs.IsGood())
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;

	return CELL_OK;
}

int cellSpursSetGlobalExceptionEventHandler(mem_ptr_t<CellSpurs> spurs, 
											mem_func_ptr_t<CellSpursGlobalExceptionEventHandler> eaHandler, mem_ptr_t<void> arg)
{
	cellSpurs.Error("cellSpursEnableExceptionEventHandler(spurs_addr=0x%x, eaHandler_addr=0x%x, arg_addr=0x%x,)", 
		spurs.GetAddr(), eaHandler.GetAddr(), arg.GetAddr());
	
	if(!spurs.IsGood() || eaHandler.IsGood())
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;

	return CELL_OK;
}

int cellSpursUnsetGlobalExceptionEventHandler(mem_ptr_t<CellSpurs> spurs)
{
	cellSpurs.Error("cellSpursUnsetGlobalExceptionEventHandler(spurs_addr=0x%x)", spurs.GetAddr());
	
	if(!spurs.IsGood())
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;

	return CELL_OK;
}

int cellSpursGetInfo(mem_ptr_t<CellSpurs> spurs, mem_ptr_t<CellSpursInfo> info)
{
	cellSpurs.Error("cellSpursGetInfo(spurs_addr=0x%x, info_addr=0x%x)", spurs.GetAddr(), info.GetAddr());
	
	if(!spurs.IsGood() || !info.IsGood())
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;

	return CELL_OK;
}

int _cellSpursSendSignal(mem_ptr_t<CellSpursTaskset> taskset, u32 taskID)
{
	cellSpurs.Error("_cellSpursSendSignal(taskset_addr=0x%x, taskID=%u)", taskset.GetAddr(), taskID);
	
	if(!taskset.IsGood())
		return CELL_SPURS_CORE_ERROR_NULL_POINTER;

	return CELL_OK;
}

int cellSpursCreateTaskset(mem_ptr_t<CellSpurs> spurs, mem_ptr_t<CellSpursTaskset> taskset,
						   u64 args, mem8_t priority, u32 maxContention)
{
	cellSpurs.Error("cellSpursCreateTaskset(spurs_addr=0x%x, taskset_addr=0x%x, args=0x%x, priority_addr=0x%x, maxContention=%u)",
		spurs.GetAddr(), taskset.GetAddr(), args, priority.GetAddr(), maxContention);
	
	if(!spurs.IsGood() || !taskset.IsGood())
		return CELL_SPURS_TASK_ERROR_NULL_POINTER;

	return CELL_OK;
}

int cellSpursJoinTaskset(mem_ptr_t<CellSpursTaskset> taskset)
{
	cellSpurs.Error("cellSpursJoinTaskset(taskset_addr=0x%x)", taskset.GetAddr());
	
	if(!taskset.IsGood())
		return CELL_SPURS_TASK_ERROR_NULL_POINTER;

	return CELL_OK;
}

int cellSpursGetTasksetId(mem_ptr_t<CellSpursTaskset> taskset, mem32_t workloadId)
{
	cellSpurs.Error("cellSpursGetTasksetId(taskset_addr=0x%x, workloadId_addr=0x%x)", taskset.GetAddr(), workloadId.GetAddr());
	
	if(!taskset.IsGood() || !workloadId.IsGood())
		return CELL_SPURS_TASK_ERROR_NULL_POINTER;

	return CELL_OK;
}

int cellSpursShutdownTaskset(mem_ptr_t<CellSpursTaskset> taskset)
{
	cellSpurs.Error("cellSpursShutdownTaskset(taskset_addr=0x%x)", taskset.GetAddr());
	
	if(!taskset.IsGood())
		return CELL_SPURS_TASK_ERROR_NULL_POINTER;

	return CELL_OK;
}

int cellSpursCreateTask(mem_ptr_t<CellSpursTaskset> taskset, mem32_t taskID, mem_ptr_t<void> elf_addr,
						mem_ptr_t<void> context_addr, u32 context_size, mem_ptr_t<CellSpursTaskLsPattern> lsPattern,
						mem_ptr_t<CellSpursTaskArgument> argument)
{
	cellSpurs.Error("cellSpursCreateTask(taskset_addr=0x%x, taskID_addr=0x%x, elf_addr_addr=0x%x, context_addr_addr=0x%x, context_size=%u, lsPattern_addr=0x%x, argument_addr=0x%x)",
		taskset.GetAddr(), taskID.GetAddr(), elf_addr.GetAddr(), context_addr.GetAddr(), context_size, lsPattern.GetAddr(), argument.GetAddr());
	
	if(!taskset.IsGood())
		return CELL_SPURS_TASK_ERROR_NULL_POINTER;

	return CELL_OK;
}

void cellSpurs_init()
{
	//libspurs core functions
	cellSpurs.AddFunc(0xacfc8dbc, cellSpursInitialize);
	cellSpurs.AddFunc(0xaa6269a8, cellSpursInitializeWithAttribute);
	cellSpurs.AddFunc(0x30aa96c4, cellSpursInitializeWithAttribute2);
	cellSpurs.AddFunc(0xca4c4600, cellSpursFinalize);

	cellSpurs.AddFunc(0x95180230, _cellSpursAttributeInitialize);
	cellSpurs.AddFunc(0x82275c1c, cellSpursAttributeSetMemoryContainerForSpuThread);
	cellSpurs.AddFunc(0x07529113, cellSpursAttributeSetNamePrefix);
	cellSpurs.AddFunc(0x1051d134, cellSpursAttributeEnableSpuPrintfIfAvailable);
	cellSpurs.AddFunc(0xa839a4d9, cellSpursAttributeSetSpuThreadGroupType);
	cellSpurs.AddFunc(0x9dcbcb5d, cellSpursAttributeEnableSystemWorkload);

	cellSpurs.AddFunc(0x39c173fb, cellSpursGetSpuThreadGroupId);
	cellSpurs.AddFunc(0xc56defb5, cellSpursGetNumSpuThread);
	cellSpurs.AddFunc(0x6c960f6d, cellSpursGetSpuThreadId);
	cellSpurs.AddFunc(0x84d2f6d5, cellSpursSetMaxContention);

	cellSpurs.AddFunc(0x80a29e27, cellSpursSetPriorities);
	cellSpurs.AddFunc(0x4de203e2, cellSpursSetPreemptionVictimHints);
	
	cellSpurs.AddFunc(0xb9bc6207, cellSpursAttachLv2EventQueue);
	cellSpurs.AddFunc(0x4e66d483, cellSpursDetachLv2EventQueue);
	//cellSpurs.AddFunc(0x5ef96465, _cellSpursEventFlagInitialize);
	cellSpurs.AddFunc(0x87630976, cellSpursEventFlagAttachLv2EventQueue);
	cellSpurs.AddFunc(0x22aab31d, cellSpursEventFlagDetachLv2EventQueue);
	cellSpurs.AddFunc(0x373523d4, cellSpursEventFlagWait);

	cellSpurs.AddFunc(0x32b94add, cellSpursEnableExceptionEventHandler);
	cellSpurs.AddFunc(0x7517724a, cellSpursSetGlobalExceptionEventHandler);
	cellSpurs.AddFunc(0x861237f8, cellSpursUnsetGlobalExceptionEventHandler);
	
	cellSpurs.AddFunc(0x1f402f8f, cellSpursGetInfo);
	
	cellSpurs.AddFunc(0xe0a6dbe4, _cellSpursSendSignal);

	//libspurs task functions
	cellSpurs.AddFunc(0x52cc6c82, cellSpursCreateTaskset);
	cellSpurs.AddFunc(0x9f72add3, cellSpursJoinTaskset);
	cellSpurs.AddFunc(0xe7dd87e1, cellSpursGetTasksetId);
	cellSpurs.AddFunc(0xa789e631, cellSpursShutdownTaskset);
	cellSpurs.AddFunc(0xbeb600ac, cellSpursCreateTask);
}