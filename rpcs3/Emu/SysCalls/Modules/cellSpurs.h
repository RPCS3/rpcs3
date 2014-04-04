#pragma once
#include "Emu/Cell/SPURSManager.h"

// Core return codes.
enum
{
	CELL_SPURS_CORE_ERROR_AGAIN        = 0x80410701,
	CELL_SPURS_CORE_ERROR_INVAL        = 0x80410702,
	CELL_SPURS_CORE_ERROR_NOMEM        = 0x80410704,
	CELL_SPURS_CORE_ERROR_SRCH         = 0x80410705,
	CELL_SPURS_CORE_ERROR_PERM         = 0x80410709,
	CELL_SPURS_CORE_ERROR_BUSY         = 0x8041070A,
	CELL_SPURS_CORE_ERROR_STAT         = 0x8041070F,
	CELL_SPURS_CORE_ERROR_ALIGN        = 0x80410710,
	CELL_SPURS_CORE_ERROR_NULL_POINTER = 0x80410711,
};

// Task return codes.
enum
{
	CELL_SPURS_TASK_ERROR_AGAIN        = 0x80410901,
	CELL_SPURS_TASK_ERROR_INVAL        = 0x80410902,
	CELL_SPURS_TASK_ERROR_NOMEM        = 0x80410904,
	CELL_SPURS_TASK_ERROR_SRCH         = 0x80410905,
	CELL_SPURS_TASK_ERROR_NOEXEC       = 0x80410907,
	CELL_SPURS_TASK_ERROR_PERM         = 0x80410909,
	CELL_SPURS_TASK_ERROR_BUSY         = 0x8041090A,
	CELL_SPURS_TASK_ERROR_FAULT        = 0x8041090D,
	CELL_SPURS_TASK_ERROR_STAT         = 0x8041090F,
	CELL_SPURS_TASK_ERROR_ALIGN        = 0x80410910,
	CELL_SPURS_TASK_ERROR_NULL_POINTER = 0x80410911,
	CELL_SPURS_TASK_ERROR_FATAL        = 0x80410914,
	CELL_SPURS_TASK_ERROR_SHUTDOWN     = 0x80410920, 
};

// Core CellSpurs structures.
struct CellSpurs
{ 
	SPURSManager *spurs;  
};

struct CellSpurs2
{ 
	SPURSManager *spurs;
};

struct CellSpursAttribute
{ 
	SPURSManagerAttribute *attr;  
};

struct CellSpursInfo
{ 
	be_t<s32> nSpus;
	be_t<s32> spuThreadGroupPriority;
	be_t<s32> ppuThreadPriority; 
	bool exitIfNoWork; 
	bool spurs2; 
	be_t<u32> traceBuffer_addr;     //void *traceBuffer;
	be_t<u64> traceBufferSize; 
	be_t<u32> traceMode; 
	be_t<u32> spuThreadGroup;       //typedef u32 sys_spu_thread_group_t;
	be_t<u32> spuThreads[8];        //typedef u32 sys_spu_thread_t;
	be_t<u32> spursHandlerThread0; 
	be_t<u32> spursHandlerThread1; 
	s8 namePrefix[CELL_SPURS_NAME_MAX_LENGTH+1]; 
	be_t<u64> namePrefixLength; 
	be_t<u32> deadlineMissCounter; 
	be_t<u32> deadlineMeetCounter; 
	//u8 padding[]; 
};

struct CellSpursExceptionInfo 
{ 
	be_t<u32> spu_thread; 
	be_t<u32> spu_npc; 
	be_t<u32> cause; 
	be_t<u64> option; 
};

struct CellSpursTraceInfo
{ 
	be_t<u32> spu_thread[8]; 
	be_t<u32> count[8]; 
	be_t<u32> spu_thread_grp; 
	be_t<u32> nspu; 
	//u8 padding[]; 
};

struct CellTraceHeader 
{ 
	u8 tag; 
	u8 length; 
	u8 cpu; 
	u8 thread; 
	be_t<u32> time; 
};

struct CellSpursTracePacket
{ 
	struct header_struct
	{ 
		u8 tag; 
		u8 length; 
		u8 spu; 
		u8 workload; 
		be_t<u32> time; 
	} header;

	struct data_struct
	{
		struct load_struct
		{ 
			be_t<u32> ea; 
			be_t<u16> ls; 
			be_t<u16> size; 
		} load; 

		struct map_struct
		{ 
			be_t<u32> offset; 
			be_t<u16> ls; 
			be_t<u16> size; 
		} map; 

		struct start_struct
		{ 
			s8 module[4];
			be_t<u16> level; 
			be_t<u16> ls; 
		} start; 

		be_t<u64> user; 
		be_t<u64> guid;
	} data;
};

// cellSpurs taskset structures.
struct CellSpursTaskset 
{
	u8 skip[6400];
};

// Exception handlers.
typedef void (*CellSpursGlobalExceptionEventHandler)(mem_ptr_t<CellSpurs> spurs, const mem_ptr_t<CellSpursExceptionInfo> info, 
													 u32 id, mem_ptr_t<void> arg);

typedef void (*CellSpursTasksetExceptionEventHandler)(mem_ptr_t<CellSpurs> spurs, mem_ptr_t<CellSpursTaskset> taskset, 
													 u32 idTask, const mem_ptr_t<CellSpursExceptionInfo> info, mem_ptr_t<void> arg);

struct CellSpursTasksetInfo 
{ 
	//CellSpursTaskInfo taskInfo[CELL_SPURS_MAX_TASK]; 
	be_t<u64> argument; 
	be_t<u32> idWorkload; 
	be_t<u32> idLastScheduledTask; //typedef unsigned CellSpursTaskId
	be_t<u32> name_addr; 
	CellSpursTasksetExceptionEventHandler exceptionEventHandler; 
	be_t<u32> exceptionEventHandlerArgument_addr; //void *exceptionEventHandlerArgument
	be_t<u64> sizeTaskset; 
	//be_t<u8> reserved[]; 
};

struct CellSpursTaskset2 
{
	be_t<u8> skip[10496];
};

struct CellSpursTasksetAttribute2 
{ 
	be_t<u32> revision; 
	be_t<u32> name_addr; 
	be_t<u64> argTaskset; 
	u8 priority[8]; 
	be_t<u32> maxContention; 
	be_t<s32> enableClearLs; 
	be_t<s32> CellSpursTaskNameBuffer_addr; //??? *taskNameBuffer
	//be_t<u32> __reserved__[]; 
};

// cellSpurs task structures.
struct CellSpursTaskNameBuffer 
{ 
	char taskName[CELL_SPURS_MAX_TASK][CELL_SPURS_MAX_TASK_NAME_LENGTH]; 
};

struct CellSpursTraceTaskData 
{ 
	be_t<u32> incident; 
	be_t<u32> task; 
};

struct CellSpursTaskArgument 
{ 
	be_t<u32> u32[4]; 
	be_t<u64> u64[2]; 
};

struct CellSpursTaskLsPattern 
{
	be_t<u32> u32[4];
	be_t<u64> u64[2];
};

struct CellSpursTaskAttribute2 
{ 
	be_t<u32> revision; 
	be_t<u32> sizeContext; 
	be_t<u64> eaContext; 
	CellSpursTaskLsPattern lsPattern; //???
	be_t<u32> name_addr; 
	//be_t<u32> __reserved__[]; 
};

struct CellSpursTaskExitCode 
{
	unsigned char skip[128];
};

struct CellSpursTaskInfo 
{ 
	CellSpursTaskLsPattern lsPattern; 
	CellSpursTaskArgument argument; 
	const be_t<u32> eaElf_addr; //void *eaElf
	const be_t<u32> eaContext_addr; //void *eaContext
	be_t<u32> sizeContext; 
	be_t<u8> state; 
	be_t<u8> hasSignal; 
	const be_t<u32> CellSpursTaskExitCode_addr; 
	u8 guid[8]; 
	//be_t<u8> reserved[]; 
};

struct CellSpursTaskBinInfo 
{ 
	be_t<u64> eaElf;
	be_t<u32> sizeContext;
	be_t<u32> __reserved__;
	CellSpursTaskLsPattern lsPattern;
};

// cellSpurs event flag.
struct CellSpursEventFlag {
	u8 skip[128];
};