#pragma once

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

//
enum
{
	CELL_SPURS_POLICY_MODULE_ERROR_AGAIN        = 0x80410801,
	CELL_SPURS_POLICY_MODULE_ERROR_INVAL        = 0x80410802,
	CELL_SPURS_POLICY_MODULE_ERROR_NOSYS        = 0x80410803,
	CELL_SPURS_POLICY_MODULE_ERROR_NOMEM        = 0x80410804,
	CELL_SPURS_POLICY_MODULE_ERROR_SRCH         = 0x80410805,
	CELL_SPURS_POLICY_MODULE_ERROR_NOENT        = 0x80410806,
	CELL_SPURS_POLICY_MODULE_ERROR_NOEXEC       = 0x80410807,
	CELL_SPURS_POLICY_MODULE_ERROR_DEADLK       = 0x80410808,
	CELL_SPURS_POLICY_MODULE_ERROR_PERM         = 0x80410809,
	CELL_SPURS_POLICY_MODULE_ERROR_BUSY         = 0x8041080A,
	CELL_SPURS_POLICY_MODULE_ERROR_ABORT        = 0x8041080C,
	CELL_SPURS_POLICY_MODULE_ERROR_FAULT        = 0x8041080D,
	CELL_SPURS_POLICY_MODULE_ERROR_CHILD        = 0x8041080E,
	CELL_SPURS_POLICY_MODULE_ERROR_STAT         = 0x8041080F,
	CELL_SPURS_POLICY_MODULE_ERROR_ALIGN        = 0x80410810,
	CELL_SPURS_POLICY_MODULE_ERROR_NULL_POINTER = 0x80410811,
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

enum
{
	CELL_SPURS_JOB_ERROR_AGAIN               = 0x80410A01,
	CELL_SPURS_JOB_ERROR_INVAL               = 0x80410A02,
	CELL_SPURS_JOB_ERROR_NOSYS               = 0x80410A03,
	CELL_SPURS_JOB_ERROR_NOMEM               = 0x80410A04,
	CELL_SPURS_JOB_ERROR_SRCH                = 0x80410A05,
	CELL_SPURS_JOB_ERROR_NOENT               = 0x80410A06,
	CELL_SPURS_JOB_ERROR_NOEXEC              = 0x80410A07,
	CELL_SPURS_JOB_ERROR_DEADLK              = 0x80410A08,
	CELL_SPURS_JOB_ERROR_PERM                = 0x80410A09,
	CELL_SPURS_JOB_ERROR_BUSY                = 0x80410A0A,
	CELL_SPURS_JOB_ERROR_JOB_DESCRIPTOR      = 0x80410A0B,
	CELL_SPURS_JOB_ERROR_JOB_DESCRIPTOR_SIZE = 0x80410A0C,
	CELL_SPURS_JOB_ERROR_FAULT               = 0x80410A0D,
	CELL_SPURS_JOB_ERROR_CHILD               = 0x80410A0E,
	CELL_SPURS_JOB_ERROR_STAT                = 0x80410A0F,
	CELL_SPURS_JOB_ERROR_ALIGN               = 0x80410A10,
	CELL_SPURS_JOB_ERROR_NULL_POINTER        = 0x80410A11,
	CELL_SPURS_JOB_ERROR_MEMORY_CORRUPTED    = 0x80410A12,

	CELL_SPURS_JOB_ERROR_MEMORY_SIZE         = 0x80410A17,
	CELL_SPURS_JOB_ERROR_UNKNOWN_COMMAND     = 0x80410A18,
	CELL_SPURS_JOB_ERROR_JOBLIST_ALIGNMENT   = 0x80410A19,
	CELL_SPURS_JOB_ERROR_JOB_ALIGNMENT       = 0x80410A1a,
	CELL_SPURS_JOB_ERROR_CALL_OVERFLOW       = 0x80410A1b,
	CELL_SPURS_JOB_ERROR_ABORT               = 0x80410A1c,
	CELL_SPURS_JOB_ERROR_DMALIST_ELEMENT     = 0x80410A1d,
	CELL_SPURS_JOB_ERROR_NUM_CACHE           = 0x80410A1e,
	CELL_SPURS_JOB_ERROR_INVALID_BINARY      = 0x80410A1f,
};

// SPURS defines.
enum SPURSKernelInterfaces
{
	CELL_SPURS_MAX_SPU = 8,
	CELL_SPURS_MAX_WORKLOAD = 16,
	CELL_SPURS_MAX_WORKLOAD2 = 32,
	CELL_SPURS_SYS_SERVICE_WORKLOAD_ID = 32,
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

enum SpursAttrFlags : u32
{
	SAF_NONE = 0x0,

	SAF_EXIT_IF_NO_WORK = 0x1,
	SAF_UNKNOWN_FLAG_30 = 0x2,
	SAF_SECOND_VERSION  = 0x4,

	SAF_UNKNOWN_FLAG_9                = 0x00400000,
	SAF_UNKNOWN_FLAG_8                = 0x00800000,
	SAF_UNKNOWN_FLAG_7                = 0x01000000,
	SAF_SYSTEM_WORKLOAD_ENABLED       = 0x02000000,
	SAF_SPU_PRINTF_ENABLED            = 0x10000000,
	SAF_SPU_TGT_EXCLUSIVE_NON_CONTEXT = 0x20000000,
	SAF_SPU_MEMORY_CONTAINER_SET      = 0x40000000,
	SAF_UNKNOWN_FLAG_0                = 0x80000000,
};

enum SpursFlags1 : u8
{
	SF1_NONE = 0x0,

	SF1_32_WORKLOADS    = 0x40,
	SF1_EXIT_IF_NO_WORK = 0x80,
};

enum SpursWorkloadConstants : u64
{
	// Workload states
	SPURS_WKL_STATE_NON_EXISTENT    = 0,
	SPURS_WKL_STATE_PREPARING       = 1,
	SPURS_WKL_STATE_RUNNABLE        = 2,
	SPURS_WKL_STATE_SHUTTING_DOWN   = 3,
	SPURS_WKL_STATE_REMOVABLE       = 4,
	SPURS_WKL_STATE_INVALID         = 5,

	// GUID
	SPURS_GUID_SYS_WKL              = 0x1BB841BF38F89D33ull,

	// Image addresses
	SPURS_IMG_ADDR_SYS_SRV_WORKLOAD = 0x100,
};

enum CellSpursModulePollStatus
{
	CELL_SPURS_MODULE_POLL_STATUS_READYCOUNT  = 1,
	CELL_SPURS_MODULE_POLL_STATUS_SIGNAL      = 2,
	CELL_SPURS_MODULE_POLL_STATUS_FLAG        = 4
};

enum SpursTraceConstants
{
	// Trace tag types
	CELL_SPURS_TRACE_TAG_KERNEL     = 0x20,
	CELL_SPURS_TRACE_TAG_SERVICE    = 0x21,
	CELL_SPURS_TRACE_TAG_TASK       = 0x22,
	CELL_SPURS_TRACE_TAG_JOB        = 0x23,
	CELL_SPURS_TRACE_TAG_OVIS       = 0x24,
	CELL_SPURS_TRACE_TAG_LOAD       = 0x2a,
	CELL_SPURS_TRACE_TAG_MAP        = 0x2b,
	CELL_SPURS_TRACE_TAG_START      = 0x2c,
	CELL_SPURS_TRACE_TAG_STOP       = 0x2d,
	CELL_SPURS_TRACE_TAG_USER       = 0x2e,
	CELL_SPURS_TRACE_TAG_GUID       = 0x2f,

	// Service incident
	CELL_SPURS_TRACE_SERVICE_INIT   = 0x01,
	CELL_SPURS_TRACE_SERVICE_WAIT   = 0x02,
	CELL_SPURS_TRACE_SERVICE_EXIT   = 0x03,

	// Trace mode flags
	CELL_SPURS_TRACE_MODE_FLAG_WRAP_BUFFER              = 0x1,
	CELL_SPURS_TRACE_MODE_FLAG_SYNCHRONOUS_START_STOP   = 0x2,
	CELL_SPURS_TRACE_MODE_FLAG_MASK                     = 0x3,
};

// SPURS task constants
enum SpursTaskConstants
{
	CELL_SPURS_MAX_TASK                     = 128,
	CELL_SPURS_TASK_TOP                     = 0x3000,
	CELL_SPURS_TASK_BOTTOM                  = 0x40000,
	CELL_SPURS_MAX_TASK_NAME_LENGTH         = 32,
	CELL_SPURS_TASK_ATTRIBUTE_REVISION      = 1,
	CELL_SPURS_TASKSET_ATTRIBUTE_REVISION   = 1,
	CELL_SPURS_TASK_EXECUTION_CONTEXT_SIZE  = 1024,
};

class SPURSManager;
class SPURSManagerEventFlag;
class SPURSManagerTaskset;
struct CellSpurs;

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

struct CellSpursWorkloadFlag
{
	be_t<u64> unused0;
	be_t<u32> unused1;
	atomic_t<u32> flag;
};

typedef void(*CellSpursShutdownCompletionEventHook)(vm::ptr<CellSpurs>, u32 wid, vm::ptr<void> arg);

struct CellSpursTraceInfo
{
	static const u32 size = 0x80;
	static const u32 align = 16;

	be_t<u32> spu_thread[8];    // 0x00
	be_t<u32> count[8];         // 0x20
	be_t<u32> spu_thread_grp;   // 0x40
	be_t<u32> nspu;             // 0x44
	//u8 padding[];
};

struct CellSpursTracePacket
{
	static const u32 size = 16;

	struct
	{
		u8 tag;
		u8 length;
		u8 spu;
		u8 workload;
		be_t<u32> time;
	} header;

	union
	{
		struct
		{
			be_t<u32> incident;
			be_t<u32> reserved;
		} service;

		struct
		{
			be_t<u32> ea;
			be_t<u16> ls;
			be_t<u16> size;
		} load;

		struct
		{
			be_t<u32> offset;
			be_t<u16> ls;
			be_t<u16> size;
		} map;

		struct
		{
			s8 module[4];
			be_t<u16> level;
			be_t<u16> ls;
		} start;

		be_t<u64> user;
		be_t<u64> guid;
		be_t<u64> stop;
	} data;
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
		u8 unk0[0x20]; // 0x00 - SPU exceptionh handler 0x08 - SPU exception handler args
		be_t<u64> sem; // 0x20
		u8 unk1[0x8];
		vm::bptr<CellSpursShutdownCompletionEventHook, 1, u64> hook; // 0x30
		vm::bptr<void, 1, u64> hookArg; // 0x38
		u8 unk2[0x40];
	};

	static_assert(sizeof(_sub_str1) == 0x80, "Wrong _sub_str1 size");

	struct _sub_str2 // Event port multiplexer
	{
		be_t<u32> unk0; // 0x00 Outstanding requests
		be_t<u32> unk1; // 0x04
		be_t<u32> unk2; // 0x08
		be_t<u32> unk3; // 0x0C
		be_t<u64> port; // 0x10
		u8 unk_[0x68];  // 0x18 - The first u64 seems to be the start of a linked list. The linked list struct seems to be {u64 next; u64 data; u64 handler}
	};

	static_assert(sizeof(_sub_str2) == 0x80, "Wrong _sub_str2 size");

	struct WorkloadInfo
	{
		vm::bptr<const void, 1, u64> addr; // Address of the executable
		be_t<u64> arg; // spu argument
		be_t<u32> size;
		atomic_t<u8> uniqueId; // The unique id is the same for all workloads with the same addr
		u8 priority[8];
	};

	static_assert(sizeof(WorkloadInfo) == 0x20, "Wrong WorkloadInfo size");

	struct _sub_str4
	{
		static const uint size = 0x10;

		vm::bptr<const char, 1, u64> nameClass;
		vm::bptr<const char, 1, u64> nameInstance;
	};

	union
	{
		// raw data
		u8 _u8[size];
		std::array<be_t<u32>, size / sizeof(u32)> _u32;

		// real data
		struct
		{
			atomic_t<u8> wklReadyCount1[0x10];                  // 0x00 Number of SPUs requested by each workload (0..15 wids).
			atomic_t<u8> wklIdleSpuCountOrReadyCount2[0x10];    // 0x10 SPURS1: Number of idle SPUs requested by each workload (0..15 wids). SPURS2: Number of SPUs requested by each workload (16..31 wids).
			u8 wklCurrentContention[0x10];                      // 0x20 Number of SPUs used by each workload. SPURS1: index = wid. SPURS2: packed 4-bit data, index = wid % 16, internal index = wid / 16.
			u8 wklPendingContention[0x10];                      // 0x30 Number of SPUs that are pending to context switch to the workload. SPURS1: index = wid. SPURS2: packed 4-bit data, index = wid % 16, internal index = wid / 16.
			u8 wklMinContention[0x10];                          // 0x40 Min SPUs required for each workload. SPURS1: index = wid. SPURS2: Unused.
			atomic_t<u8> wklMaxContention[0x10];                // 0x50 Max SPUs that may be allocated to each workload. SPURS1: index = wid. SPURS2: packed 4-bit data, index = wid % 16, internal index = wid / 16.
			CellSpursWorkloadFlag wklFlag;                      // 0x60
			atomic_t<u16> wklSignal1;                           // 0x70 (bitset for 0..15 wids)
			atomic_t<u8> sysSrvMessage;                         // 0x72
			u8 spuIdling;                                       // 0x73
			u8 flags1;                                          // 0x74 Type is SpursFlags1
			u8 sysSrvTraceControl;                              // 0x75
			u8 nSpus;                                           // 0x76
			atomic_t<u8> wklFlagReceiver;                       // 0x77
			atomic_t<u16> wklSignal2;                           // 0x78 (bitset for 16..32 wids)
			u8 x7A[6];                                          // 0x7A
			atomic_t<u8> wklState1[0x10];                       // 0x80 SPURS_WKL_STATE_*
			u8 wklStatus1[0x10];                                // 0x90
			u8 wklEvent1[0x10];                                 // 0xA0
			atomic_t<u32> wklMskA;                              // 0xB0 - System service - Available workloads (32*u1)
			atomic_t<u32> wklMskB;                              // 0xB4 - System service - Available module id
			u32 xB8;                                            // 0xB8
			u8 sysSrvExitBarrier;                               // 0xBC
			atomic_t<u8> sysSrvMsgUpdateWorkload;               // 0xBD
			u8 xBE;                                             // 0xBE
			u8 sysSrvMsgTerminate;                              // 0xBF
			u8 sysSrvWorkload[8];                               // 0xC0
			u8 sysSrvOnSpu;                                     // 0xC8
			u8 spuPort;                                         // 0xC9
			u8 xCA;                                             // 0xCA
			u8 xCB;                                             // 0xCB
			u8 xCC;                                             // 0xCC
			u8 xCD;                                             // 0xCD
			u8 sysSrvMsgUpdateTrace;                            // 0xCE
			u8 xCF;                                             // 0xCF
			atomic_t<u8> wklState2[0x10];                       // 0xD0 SPURS_WKL_STATE_*
			u8 wklStatus2[0x10];                                // 0xE0
			u8 wklEvent2[0x10];                                 // 0xF0
			_sub_str1 wklF1[0x10];                              // 0x100
			vm::bptr<CellSpursTraceInfo, 1, u64> traceBuffer;   // 0x900
			be_t<u32> traceStartIndex[6];                       // 0x908
			u8 unknown7[0x948 - 0x920];                         // 0x920
			be_t<u64> traceDataSize;                            // 0x948
			be_t<u32> traceMode;                                // 0x950
			u8 unknown8[0x980 - 0x954];                         // 0x954
			be_t<u64> semPrv;     // 0x980
			be_t<u32> unk11;      // 0x988
			be_t<u32> unk12;      // 0x98C
			be_t<u64> unk13;      // 0x990
			u8 unknown4[0xB00 - 0x998];
			WorkloadInfo wklInfo1[0x10]; // 0xB00
			WorkloadInfo wklInfoSysSrv;  // 0xD00
			be_t<u64> ppu0;       // 0xD20
			be_t<u64> ppu1;       // 0xD28
			be_t<u32> spuTG;      // 0xD30 - SPU thread group
			be_t<u32> spus[8];    // 0xD34
			u8 unknown3[0xD5C - 0xD54];
			be_t<u32> queue;      // 0xD5C - Event queue
			be_t<u32> port;       // 0xD60 - Event port
			atomic_t<u8> xD64;    // 0xD64 - SPURS handler dirty
			atomic_t<u8> xD65;    // 0xD65 - SPURS handler waiting
			atomic_t<u8> xD66;    // 0xD66 - SPURS handler exiting
			atomic_t<u32> enableEH; // 0xD68
			be_t<u32> exception;  // 0xD6C
			sys_spu_image spuImg; // 0xD70
			be_t<u32> flags;      // 0xD80
			be_t<s32> spuPriority; // 0xD84
			be_t<u32> ppuPriority; // 0xD88
			char prefix[0x0f];    // 0xD8C
			u8 prefixSize;        // 0xD9B
			be_t<u32> unk5;       // 0xD9C
			be_t<u32> revision;   // 0xDA0
			be_t<u32> sdkVersion; // 0xDA4
			atomic_t<u64> spups;  // 0xDA8 - SPU port bits
			sys_lwmutex_t mutex;  // 0xDB0
			sys_lwcond_t cond;    // 0xDC8
			u8 unknown9[0xE00 - 0xDD0];
			_sub_str4 wklH1[0x10]; // 0xE00
			_sub_str2 sub3;       // 0xF00
			u8 unknown6[0x1000 - 0xF80]; // 0xF80 - Gloabl SPU exception handler 0xF88 - Gloabl SPU exception handlers args
			WorkloadInfo wklInfo2[0x10]; // 0x1000
			_sub_str1 wklF2[0x10]; // 0x1200
			_sub_str4 wklH2[0x10]; // 0x1A00
		} m;

		// alternative implementation
		struct
		{
			SPURSManager *spurs;
		} c;
	};

	__forceinline atomic_t<u8>& wklState(const u32 wid)
	{
		if (wid & 0x10)
		{
			return m.wklState2[wid & 0xf];
		}
		else
		{
			return m.wklState1[wid & 0xf];
		}
	}

	__forceinline vm::ptr<sys_lwmutex_t> get_lwmutex()
	{
		return vm::ptr<sys_lwmutex_t>::make(Memory.RealToVirtualAddr(&m.mutex));
	}

	__forceinline vm::ptr<sys_lwcond_t> get_lwcond()
	{
		return vm::ptr<sys_lwcond_t>::make(Memory.RealToVirtualAddr(&m.cond));
	}
};

typedef CellSpurs CellSpurs2;

struct CellSpursWorkloadAttribute
{
	static const uint align = 8;
	static const uint size = 512;

	union
	{
		// raw data
		u8 _u8[size];

		// real data
		struct
		{
			be_t<u32> revision;
			be_t<u32> sdkVersion;
			vm::bptr<const void> pm;
			be_t<u32> size;
			be_t<u64> data;
			u8 priority[8];
			be_t<u32> minContention;
			be_t<u32> maxContention;
			vm::bptr<const char> nameClass;
			vm::bptr<const char> nameInstance;
			vm::bptr<CellSpursShutdownCompletionEventHook> hook;
			vm::bptr<void> hookArg;
		} m;
	};
};

struct CellSpursEventFlag
{
	SPURSManagerEventFlag *eventFlag;
};

union CellSpursTaskArgument
{
	be_t<u32> u32[4];
	be_t<u64> u64[2];
};

union CellSpursTaskLsPattern
{
	be_t<u32> u32[4];
	be_t<u64> u64[2];
};

struct CellSpursTaskset
{
	static const u32 align = 128;
	static const u32 size = 6400;

	struct TaskInfo
	{
		CellSpursTaskArgument args;
		vm::bptr<u64, 1, u64> elf_addr;
		vm::bptr<u64, 1, u64> context_save_storage; // This is ((context_save_storage_addr & 0xFFFFFFF8) | allocated_ls_blocks)
		CellSpursTaskLsPattern ls_pattern;
	};

	static_assert(sizeof(TaskInfo) == 0x30, "Wrong TaskInfo size");

	union
	{
		// Raw data
		u8 _u8[size];

		// Real data
		struct _CellSpursTaskset
		{
			be_t<u32> running_set[4];                    // 0x00
			be_t<u32> ready_set[4];                      // 0x10
			be_t<u32> unk_set[4];                        // 0x20 - TODO: Find out what this is
			be_t<u32> enabled_set[4];                    // 0x30
			be_t<u32> signal_received_set[4];            // 0x40
			be_t<u32> waiting_set[4];                    // 0x50
			vm::bptr<CellSpurs, 1, u64> spurs;           // 0x60
			be_t<u64> args;                              // 0x68
			u8 enable_clear_ls;                          // 0x70
			u8 x71;                                      // 0x71
			u8 x72;                                      // 0x72
			u8 last_scheduled_task;                      // 0x73
			be_t<u32> wid;                               // 0x74
			u8 unk1[8];                                  // 0x78
			TaskInfo task_info[128];                     // 0x80
			vm::bptr<u64, 1, u64> exception_handler;     // 0x1880
			vm::bptr<u64, 1, u64> exception_handler_arg; // 0x1888
			be_t<u32> size;                              // 0x1890
			u32 unk2;                                    // 0x1894
			u32 event_flag_id1;                          // 0x1898
			u32 event_flag_id2;                          // 0x189C
			u8 unk3[0x60];                               // 0x18A0
		} m;

		static_assert(sizeof(_CellSpursTaskset) == size, "Wrong _CellSpursTaskset size");

		SPURSManagerTaskset *taskset;
	};
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

// Exception handlers.
//typedef void (*CellSpursGlobalExceptionEventHandler)(vm::ptr<CellSpurs> spurs, vm::ptr<const CellSpursExceptionInfo> info, 
//													   u32 id, vm::ptr<void> arg);
//
//typedef void (*CellSpursTasksetExceptionEventHandler)(vm::ptr<CellSpurs> spurs, vm::ptr<CellSpursTaskset> taskset, 
//													    u32 idTask, vm::ptr<const CellSpursExceptionInfo> info, vm::ptr<void> arg);

struct CellSpursTaskNameBuffer
{
	static const u32 align = 16;

	char taskName[CELL_SPURS_MAX_TASK][CELL_SPURS_MAX_TASK_NAME_LENGTH];
};

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
	static const u32 align = 128;
	static const u32 size = 10496;

	struct TaskInfo
	{
		CellSpursTaskArgument args;
		vm::bptr<u64, 1, u64> elf_addr;
		vm::bptr<u64, 1, u64> context_save_storage; // This is ((context_save_storage_addr & 0xFFFFFFF8) | allocated_ls_blocks)
		CellSpursTaskLsPattern ls_pattern;
	};

	static_assert(sizeof(TaskInfo) == 0x30, "Wrong TaskInfo size");

	union
	{
		// Raw data
		u8 _u8[size];

		// Real data
		struct _CellSpursTaskset2
		{
			be_t<u32> running_set[4];                    // 0x00
			be_t<u32> ready_set[4];                      // 0x10
			be_t<u32> unk_set[4];                        // 0x20 - TODO: Find out what this is
			be_t<u32> enabled_set[4];                    // 0x30
			be_t<u32> signal_received_set[4];            // 0x40
			be_t<u32> waiting_set[4];                    // 0x50
			vm::bptr<CellSpurs, 1, u64> spurs;           // 0x60
			be_t<u64> args;                              // 0x68
			u8 enable_clear_ls;                          // 0x70
			u8 x71;                                      // 0x71
			u8 x72;                                      // 0x72
			u8 last_scheduled_task;                      // 0x73
			be_t<u32> wid;                               // 0x74
			u8 unk1[8];                                  // 0x78
			TaskInfo task_info[128];                     // 0x80
			vm::bptr<u64, 1, u64> exception_handler;     // 0x1880
			vm::bptr<u64, 1, u64> exception_handler_arg; // 0x1888
			be_t<u32> size;                              // 0x1890
			u32 unk2;                                    // 0x1894
			u32 event_flag_id1;                          // 0x1898
			u32 event_flag_id2;                          // 0x189C
			u8 unk3[0xE8];                               // 0x18A0
			u64 task_exit_code[256];                     // 0x1988
			u8 unk4[0x778];                              // 0x2188
		} m;

		static_assert(sizeof(_CellSpursTaskset2) == size, "Wrong _CellSpursTaskset2 size");
	};
};

struct CellSpursTasksetAttribute
{
	static const u32 align = 8;
	static const u32 size = 512;

	union
	{
		// Raw data
		u8 _u8[size];

		// Real data
		struct
		{
			be_t<u32> revision;             // 0x00
			be_t<u32> sdk_version;          // 0x04
			be_t<u64> args;                 // 0x08
			u8 priority[8];                 // 0x10
			be_t<u32> max_contention;       // 0x18
			vm::bptr<const char> name;      // 0x1C
			be_t<u32> taskset_size;         // 0x20
			be_t<s32> enable_clear_ls;      // 0x24
		} m;
	};
};

struct CellSpursTasksetAttribute2
{
	static const u32 align = 8;
	static const u32 size = 512;

	union
	{
		// Raw data
		u8 _u8[size];

		// Real data
		struct
		{
			be_t<u32> revision;                                 // 0x00
			vm::bptr<const char> name;                          // 0x04
			be_t<u64> args;                                     // 0x08
			u8 priority[8];                                     // 0x10
			be_t<u32> max_contention;                           // 0x18
			be_t<s32> enable_clear_ls;                          // 0x1C
			vm::bptr<CellSpursTaskNameBuffer> task_name_buffer; // 0x20
		} m;
	};
};

struct CellSpursTraceTaskData
{
	be_t<u32> incident;
	be_t<u32> task;
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

// The SPURS kernel data store. This resides at 0x00 of the LS.
struct SpursKernelMgmtData {
	u8 tempArea[0x80];                              // 0x100
	u8 wklLocContention[0x10];                      // 0x180
	u8 wklLocPendingContention[0x10];               // 0x190
	u8 priority[0x10];                              // 0x1A0
	u8 x1B0[0x10];                                  // 0x1B0
	vm::bptr<CellSpurs, 1, u64> spurs;              // 0x1C0
	be_t<u32> spuNum;                               // 0x1C8
	be_t<u32> dmaTagId;                             // 0x1CC
	vm::bptr<const void, 1, u64> wklCurrentAddr;    // 0x1D0
	be_t<u32> wklCurrentUniqueId;                   // 0x1D8
	be_t<u32> wklCurrentId;                         // 0x1DC
	be_t<u32> yieldToKernelAddr;                    // 0x1E0
	be_t<u32> selectWorkloadAddr;                   // 0x1E4
	u8 x1E8;                                        // 0x1E8
	u8 x1E9;                                        // 0x1E9
	u8 sysSrvInitialised;                           // 0x1EA
	u8 spuIdling;                                   // 0x1EB
	be_t<u16> wklRunnable1;                         // 0x1EC
	be_t<u16> wklRunnable2;                         // 0x1EE
	u8 x1F0[0x210 - 0x1F0];                         // 0x1F0
	be_t<u64> traceBuffer;                          // 0x210
	be_t<u32> traceMsgCount;                        // 0x218
	be_t<u32> traceMaxCount;                        // 0x21C
	u8 wklUniqueId[0x10];                           // 0x220
};

s64 spursAttachLv2EventQueue(vm::ptr<CellSpurs> spurs, u32 queue, vm::ptr<u8> port, s32 isDynamic, bool wasCreated);
s64 spursWakeUp(PPUThread& CPU, vm::ptr<CellSpurs> spurs);
