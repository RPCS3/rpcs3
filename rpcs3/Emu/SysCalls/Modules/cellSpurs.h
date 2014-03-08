#pragma once

// return codes
enum
{
	CELL_SPURS_CORE_ERROR_AGAIN			= 0x80410701,
	CELL_SPURS_CORE_ERROR_INVAL			= 0x80410702,
	CELL_SPURS_CORE_ERROR_NOMEM			= 0x80410704,
	CELL_SPURS_CORE_ERROR_SRCH			= 0x80410705,
	CELL_SPURS_CORE_ERROR_PERM			= 0x80410709,
	CELL_SPURS_CORE_ERROR_BUSY			= 0x8041070A,
	CELL_SPURS_CORE_ERROR_STAT			= 0x8041070F,
	CELL_SPURS_CORE_ERROR_ALIGN			= 0x80410710,
	CELL_SPURS_CORE_ERROR_NULL_POINTER	= 0x80410711,
};

//defines
enum SPURSKernelInterfaces
{
	CELL_SPURS_MAX_SPU				= 8,
	CELL_SPURS_MAX_WORKLOAD			= 16,
	CELL_SPURS_MAX_WORKLOAD2		= 32,
	CELL_SPURS_MAX_PRIORITY			= 16,
	CELL_SPURS_NAME_MAX_LENGTH		= 15,
	CELL_SPURS_SIZE					= 4096,
	CELL_SPURS_SIZE2				= 8192,
	CELL_SPURS_ALIGN				= 128,
	CELL_SPURS_ATTRIBUTE_SIZE		= 512,
	CELL_SPURS_ATTRIBUTE_ALIGN		= 8,
	CELL_SPURS_INTERRUPT_VECTOR		= 0x0,
	CELL_SPURS_LOCK_LINE			= 0x80,
	CELL_SPURS_KERNEL_DMA_TAG_ID	= 31,
};

enum RangeofEventQueuePortNumbers
{
	CELL_SPURS_STATIC_PORT_RANGE_BOTTOM		= 15,
	CELL_SPURS_DYNAMIC_PORT_RANGE_TOP		= 16,
	CELL_SPURS_DYNAMIC_PORT_RANGE_BOTTOM	= 63,
};

enum SPURSTraceTypes
{
	CELL_SPURS_TRACE_TAG_LOAD	= 0x2a, 
	CELL_SPURS_TRACE_TAG_MAP	= 0x2b,
	CELL_SPURS_TRACE_TAG_START	= 0x2c,
	CELL_SPURS_TRACE_TAG_STOP	= 0x2d,
	CELL_SPURS_TRACE_TAG_USER	= 0x2e,
	CELL_SPURS_TRACE_TAG_GUID	= 0x2f,
};

struct CellSpursInfo
{ 
	be_t<int> nSpus; 
	be_t<int> spuThreadGroupPriority; 
	be_t<int> ppuThreadPriority; 
	bool exitIfNoWork; 
	bool spurs2; 
	be_t<u32> traceBuffer_addr;		//void *traceBuffer;
	be_t<u64> traceBufferSize; 
	be_t<u32> traceMode; 
	be_t<u32> spuThreadGroup;		//typedef u32 sys_spu_thread_group_t;
	be_t<u32> spuThreads[8];		//typedef u32 sys_spu_thread_t;
	be_t<u32> spursHandlerThread0; 
	be_t<u32> spursHandlerThread1; 
	char namePrefix[CELL_SPURS_NAME_MAX_LENGTH+1]; 
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

#ifdef WIN32
__declspec(align(8)) struct CellTraceHeader
		{
			u8 tag;
			u8 length;
			u8 cpu;
			u8 thread;
			be_t<u32> time;
		};
#else
struct CellTraceHeader
{
	u8 tag;
	u8 length;
	u8 cpu;
	u8 thread;
	be_t<u32> time;

	__attribute__ ((aligned (8)));
};


#endif


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
			char module[4]; 
			be_t<u16> level; 
			be_t<u16> ls; 
		} start; 

		be_t<u64> user; 
		be_t<u64> guid;
	} data;
};

#ifdef WIN32
__declspec(align(128)) struct CellSpurs
{ 
	u8 skip[CELL_SPURS_SIZE];  
};
#else

struct CellSpurs
{
	u8 skip[CELL_SPURS_SIZE];

	__attribute__ ((aligned (128)));

};

#endif




#ifdef WIN32
__declspec(align(128)) struct CellSpurs2
{ 
	u8 skip[CELL_SPURS_SIZE2 - CELL_SPURS_SIZE];
};
#else

struct CellSpurs2
{
	u8 skip[CELL_SPURS_SIZE2 - CELL_SPURS_SIZE];

	__attribute__ ((aligned (128)));

};

#endif




#ifdef WIN32
__declspec(align(8)) struct CellSpursAttribute
{
	u8 skip[CELL_SPURS_ATTRIBUTE_SIZE];
};
#else

struct CellSpursAttribute
{ 
	u8 skip[CELL_SPURS_ATTRIBUTE_SIZE];  
	__attribute__ ((aligned (8)));
};
#endif




//typedef unsigned CellSpursWorkloadId;

typedef void (*CellSpursGlobalExceptionEventHandler)(mem_ptr_t<CellSpurs> spurs, const mem_ptr_t<CellSpursExceptionInfo> info, 
													 uint id, mem_ptr_t<void> arg);


// task datatypes and constans
enum TaskConstants
{
	CELL_SPURS_MAX_TASK				= 128,
	CELL_SPURS_TASK_TOP				= 0x3000,
	CELL_SPURS_TASK_BOTTOM			= 0x40000,
	CELL_SPURS_MAX_TASK_NAME_LENGTH	= 32,
};

enum
{
	CELL_SPURS_TASK_ERROR_AGAIN			= 0x80410901,
	CELL_SPURS_TASK_ERROR_INVAL			= 0x80410902,
	CELL_SPURS_TASK_ERROR_NOMEM			= 0x80410904,
	CELL_SPURS_TASK_ERROR_SRCH			= 0x80410905,
	CELL_SPURS_TASK_ERROR_NOEXEC		= 0x80410907,
	CELL_SPURS_TASK_ERROR_PERM			= 0x80410909,
	CELL_SPURS_TASK_ERROR_BUSY			= 0x8041090A,
	CELL_SPURS_TASK_ERROR_FAULT			= 0x8041090D,
	CELL_SPURS_TASK_ERROR_STAT			= 0x8041090F,
	CELL_SPURS_TASK_ERROR_ALIGN			= 0x80410910,
	CELL_SPURS_TASK_ERROR_NULL_POINTER	= 0x80410911,
	CELL_SPURS_TASK_ERROR_FATAL			= 0x80410914,
	CELL_SPURS_TASK_ERROR_SHUTDOWN		= 0x80410920, 
};

#ifdef WIN32
__declspec(align(128)) struct CellSpursTaskset 
{
	u8 skip[6400];
};
#else
struct CellSpursTaskset
{
	u8 skip[6400];

	__attribute__ ((aligned (128)));
};


#endif


typedef void(*CellSpursTasksetExceptionEventHandler)(mem_ptr_t<CellSpurs> spurs, mem_ptr_t<CellSpursTaskset> taskset, 
													 uint idTask, const mem_ptr_t<CellSpursExceptionInfo> info, mem_ptr_t<void> arg);


struct CellSpursTasksetInfo 
{ 
	//CellSpursTaskInfo taskInfo[CELL_SPURS_MAX_TASK]; 
	be_t<u64> argument; 
	be_t<uint> idWorkload; 
	be_t<uint> idLastScheduledTask; //typedef unsigned CellSpursTaskId
	be_t<u32> name_addr; 
	CellSpursTasksetExceptionEventHandler exceptionEventHandler; 
	be_t<u32> exceptionEventHandlerArgument_addr; //void *exceptionEventHandlerArgument
	be_t<u64> sizeTaskset; 
	//be_t<u8> reserved[]; 
};


/*
#define CELL_SPURS_TASKSET_CLASS0_SIZE            (128 * 50)
#define	_CELL_SPURS_TASKSET_CLASS1_EXTENDED_SIZE  (128 + 128 * 16 + 128 * 15)
#define	CELL_SPURS_TASKSET_CLASS1_SIZE			  (CELL_SPURS_TASKSET_CLASS0_SIZE + _CELL_SPURS_TASKSET_CLASS1_EXTENDED_SIZE)
#define CELL_SPURS_TASKSET2_SIZE                  (CELL_SPURS_TASKSET_CLASS1_SIZE)
#define CELL_SPURS_TASKSET2_ALIGN	              128
#define CELL_SPURS_TASKSET_ALIGN                  128
#define CELL_SPURS_TASKSET_SIZE                   CELL_SPURS_TASKSET_CLASS0_SIZE
*/
#ifdef WIN32
__declspec(align(128)) struct CellSpursTaskset2
{
	be_t<u8> skip[10496];
};
#else

struct CellSpursTaskset2
{
	be_t<u8> skip[10496];

	__attribute__ ((aligned (128)));
};

#endif


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


#ifdef WIN32
__declspec(align(128)) struct CellSpursTaskExitCode 
{
	unsigned char skip[128];
};
#else

struct CellSpursTaskExitCode
{
	unsigned char skip[128];

	__attribute__ ((aligned (128)));
};

#endif


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

