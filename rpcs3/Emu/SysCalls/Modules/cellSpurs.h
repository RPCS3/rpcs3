#pragma once
#include "Emu/SysCalls/lv2/sys_lwmutex.h"
#include "Emu/SysCalls/lv2/sys_lwcond.h"
#include "Emu/SysCalls/lv2/sys_spu.h"

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
	CELL_SPURS_TASK_ERROR_ALIGN        = 0x80410910,
	CELL_SPURS_TASK_ERROR_STAT         = 0x8041090F,
	CELL_SPURS_TASK_ERROR_NULL_POINTER = 0x80410911,
	CELL_SPURS_TASK_ERROR_FATAL        = 0x80410914,
	CELL_SPURS_TASK_ERROR_SHUTDOWN     = 0x80410920,
};

// SPURS defines.
enum SPURSKernelInterfaces
{
	CELL_SPURS_MAX_SPU = 8,
	CELL_SPURS_MAX_WORKLOAD = 16,
	CELL_SPURS_MAX_WORKLOAD2 = 32,
	CELL_SPURS_MAX_PRIORITY = 16,
	CELL_SPURS_NAME_MAX_LENGTH = 15,
	CELL_SPURS_SIZE = 4096,
	CELL_SPURS_SIZE2 = 8192,
	CELL_SPURS_ALIGN = 128,
	CELL_SPURS_ATTRIBUTE_SIZE = 512,
	CELL_SPURS_ATTRIBUTE_ALIGN = 8,
	CELL_SPURS_INTERRUPT_VECTOR = 0x0,
	CELL_SPURS_LOCK_LINE = 0x80,
	CELL_SPURS_KERNEL_DMA_TAG_ID = 31,
};

enum RangeofEventQueuePortNumbers
{
	CELL_SPURS_STATIC_PORT_RANGE_BOTTOM = 15,
	CELL_SPURS_DYNAMIC_PORT_RANGE_TOP = 16,
	CELL_SPURS_DYNAMIC_PORT_RANGE_BOTTOM = 63,
};

enum SPURSTraceTypes
{
	CELL_SPURS_TRACE_TAG_LOAD = 0x2a,
	CELL_SPURS_TRACE_TAG_MAP = 0x2b,
	CELL_SPURS_TRACE_TAG_START = 0x2c,
	CELL_SPURS_TRACE_TAG_STOP = 0x2d,
	CELL_SPURS_TRACE_TAG_USER = 0x2e,
	CELL_SPURS_TRACE_TAG_GUID = 0x2f,
};

// SPURS task defines.
enum TaskConstants
{
	CELL_SPURS_MAX_TASK = 128,
	CELL_SPURS_TASK_TOP = 0x3000,
	CELL_SPURS_TASK_BOTTOM = 0x40000,
	CELL_SPURS_MAX_TASK_NAME_LENGTH = 32,
};

class SPURSManager;
class SPURSManagerEventFlag;
class SPURSManagerTaskset;

enum SpursAttrFlags : u32
{
	SAF_NONE = 0x0,
	SAF_EXIT_IF_NO_WORK = 0x1,
	SAF_SECOND_VERSION = 0x4,

	SAF_UNKNOWN_FLAG_9                = 0x00400000,
	SAF_UNKNOWN_FLAG_8                = 0x00800000,
	SAF_UNKNOWN_FLAG_7                = 0x01000000,
	SAF_SYSTEM_WORKLOAD_ENABLED       = 0x02000000,
	SAF_SPU_PRINTF_ENABLED            = 0x10000000,
	SAF_SPU_TGT_EXCLUSIVE_NON_CONTEXT = 0x20000000,
	SAF_SPU_MEMORY_CONTAINER_SET      = 0x40000000,
	SAF_UNKNOWN_FLAG_0                = 0x80000000,
};

struct CellSpursAttribute
{
	static const uint align = 8;
	static const uint size = 512;

	union
	{
		// raw data
		u8 _u8[size];
		struct { be_t<u32> _u32[size / sizeof(u32)]; };

		// real data
		struct
		{
			be_t<u32> revision;    // 0x0
			be_t<u32> sdkVersion;  // 0x4
			be_t<u32> nSpus;       // 0x8
			be_t<s32> spuPriority; // 0xC
			be_t<s32> ppuPriority; // 0x10
			bool exitIfNoWork;     // 0x14
			char prefix[15];       // 0x15 (not a NTS)
			be_t<u32> prefixSize;  // 0x24
			be_t<u32> flags;       // 0x28 (SpursAttrFlags)
			be_t<u32> container;   // 0x2C
			be_t<u32> unk0;        // 0x30
			be_t<u32> unk1;        // 0x34
			u8 swlPriority[8];     // 0x38
			be_t<u32> swlMaxSpu;   // 0x40
			be_t<u32> swlIsPreem;  // 0x44
		} m;

		// alternative implementation
		struct
		{
		} c;
	};
};

// Core CellSpurs structures
struct CellSpurs
{
	static const uint align = 128;
	static const uint size = 0x2000; // size of CellSpurs2
	static const uint size1 = 0x1000; // size of CellSpurs
	static const uint size2 = 0x1000;

	struct _sub_str1
	{
		static const uint size = 0x80;

		be_t<u64> sem;
		u8 unk_[0x78];
	};

	union
	{
		// raw data
		u8 _u8[size];
		std::array<be_t<u32>, size / sizeof(u32)> _u32;

		// real data
		struct
		{
			u8 unknown[0x6C];
			be_t<u32> unk18;      // 0x6C
			u8 unk17[4];          // 0x70
			u8 flags1;            // 0x74
			u8 unk16;             // 0x75
			u8 nSpus;             // 0x76
			u8 unk15;             // 0x77
			u8 unknown0[0xB0 - 0x78];
			be_t<u32> unk0;       // 0x0B0
			u8 unknown2[0xC0 - 0xB4];
			u8 unk6[0x10];        // 0x0C0
			u8 unknown1[0x120 - 0x0D0];
			_sub_str1 sub1[0x10]; // 0x120
			u8 unknown7[0x980 - 0x920];
			be_t<u64> semPrv;     // 0x980
			be_t<u32> unk11;      // 0x988
			be_t<u32> unk12;      // 0x98C
			be_t<u64> unk13;      // 0x990
			u8 unknown4[0xD00 - 0x998];
			be_t<u64> unk7;       // 0xD00
			be_t<u64> unk8;       // 0xD08
			be_t<u32> unk9;       // 0xD10
			u8 unk10;             // 0xD14
			u8 unknown5[0xD20 - 0xD15];
			be_t<u64> unk1;       // 0xD20
			be_t<u64> unk2;       // 0xD28
			be_t<u32> spuTG;      // 0xD30
			be_t<u32> spus[8];    // 0xD34
			u8 unknown3[0xD5C - 0xD54];
			be_t<u32> queue;      // 0xD5C
			be_t<u32> port;       // 0xD60
			u8 unk19[0xC];        // 0xD64
			sys_spu_image spuImg; // 0xD70
			be_t<u32> flags;      // 0xD80
			be_t<s32> spuPriority;// 0xD84
			be_t<u32> ppuPriority;// 0xD88
			char prefix[0x0f];    // 0xD8C
			u8 prefixSize;        // 0xD9B
			be_t<u32> unk5;       // 0xD9C
			be_t<u32> revision;   // 0xDA0
			be_t<u32> sdkVersion; // 0xDA4
			u8 unknown8[0xDB0 - 0xDA8];
			sys_lwmutex_t mutex;  // 0xDB0
			sys_lwcond_t cond;    // 0xDC8
			u8 unknown6[0x1220 - 0xDD0];
			_sub_str1 sub2[0x10]; // 0x1220
			// ...
		} m;

		// alternative implementation
		struct
		{
			SPURSManager *spurs;
		} c;
	};
};

typedef CellSpurs CellSpurs2;

struct CellSpursEventFlag
{
	SPURSManagerEventFlag *eventFlag;
};

struct CellSpursTaskset
{
	SPURSManagerTaskset *taskset;
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

// Exception handlers.
//typedef void (*CellSpursGlobalExceptionEventHandler)(vm::ptr<CellSpurs> spurs, vm::ptr<const CellSpursExceptionInfo> info, 
//													   u32 id, vm::ptr<void> arg);
//
//typedef void (*CellSpursTasksetExceptionEventHandler)(vm::ptr<CellSpurs> spurs, vm::ptr<CellSpursTaskset> taskset, 
//													    u32 idTask, vm::ptr<const CellSpursExceptionInfo> info, vm::ptr<void> arg);

struct CellSpursTasksetInfo
{
	//CellSpursTaskInfo taskInfo[CELL_SPURS_MAX_TASK];
	be_t<u64> argument;
	be_t<u32> idWorkload;
	be_t<u32> idLastScheduledTask; //typedef unsigned CellSpursTaskId
	be_t<u32> name_addr;
	be_t<u32> exceptionEventHandler_addr;
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

typedef be_t<u32> be_u32;
typedef be_t<u64> be_u64;

struct CellSpursTaskArgument
{
	be_u32 u32[4];
	be_u64 u64[2];
};

struct CellSpursTaskLsPattern
{
	be_u32 u32[4];
	be_u64 u64[2];
};

struct CellSpursTaskAttribute2
{
	be_t<u32> revision;
	be_t<u32> sizeContext;
	be_t<u64> eaContext;
	CellSpursTaskLsPattern lsPattern;
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
