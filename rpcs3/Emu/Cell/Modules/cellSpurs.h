#pragma once

#include "cellSync.h"

#include "util/v128.hpp"

#include "Emu/Cell/lv2/sys_lwmutex.h"
#include "Emu/Cell/lv2/sys_lwcond.h"
#include "Emu/Cell/lv2/sys_spu.h"

struct CellSpurs;
struct CellSpursTaskset;

// Core return codes.
enum CellSpursCoreError : u32
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
enum CellSpursPolicyModuleError : u32
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
enum CellSpursTaskError : u32
{
	CELL_SPURS_TASK_ERROR_AGAIN        = 0x80410901,
	CELL_SPURS_TASK_ERROR_INVAL        = 0x80410902,
	CELL_SPURS_TASK_ERROR_NOSYS        = 0x80410903,
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

enum CellSpursJobError : u32
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
enum SPURSKernelInterfaces : u32
{
	CELL_SPURS_MAX_SPU                      = 8,
	CELL_SPURS_MAX_WORKLOAD                 = 16,
	CELL_SPURS_MAX_WORKLOAD2                = 32,
	CELL_SPURS_SYS_SERVICE_WORKLOAD_ID      = 32,
	CELL_SPURS_MAX_PRIORITY                 = 16,
	CELL_SPURS_NAME_MAX_LENGTH              = 15,
	CELL_SPURS_SIZE                         = 4096,
	CELL_SPURS_SIZE2                        = 8192,
	CELL_SPURS_INTERRUPT_VECTOR             = 0x0,
	CELL_SPURS_LOCK_LINE                    = 0x80,
	CELL_SPURS_KERNEL_DMA_TAG_ID            = 31,
	CELL_SPURS_KERNEL1_ENTRY_ADDR           = 0x818,
	CELL_SPURS_KERNEL2_ENTRY_ADDR           = 0x848,
	CELL_SPURS_KERNEL1_EXIT_ADDR            = 0x808,
	CELL_SPURS_KERNEL2_EXIT_ADDR            = 0x838,
	CELL_SPURS_KERNEL1_SELECT_WORKLOAD_ADDR = 0x290,
	CELL_SPURS_KERNEL2_SELECT_WORKLOAD_ADDR = 0x290,
};

enum RangeofEventQueuePortNumbers
{
	CELL_SPURS_STATIC_PORT_RANGE_BOTTOM  = 15,
	CELL_SPURS_DYNAMIC_PORT_RANGE_TOP    = 16,
	CELL_SPURS_DYNAMIC_PORT_RANGE_BOTTOM = 63,
};

enum SpursAttrFlags : u32
{
	SAF_NONE                          = 0x00000000,
	SAF_EXIT_IF_NO_WORK               = 0x00000001,
	SAF_UNKNOWN_FLAG_30               = 0x00000002,
	SAF_SECOND_VERSION                = 0x00000004,
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
	SF1_NONE            = 0x00,
	SF1_32_WORKLOADS    = 0x40,
	SF1_EXIT_IF_NO_WORK = 0x80,
};

enum SpursWorkloadState : u8
{
	// Workload states
	SPURS_WKL_STATE_NON_EXISTENT    = 0,
	SPURS_WKL_STATE_PREPARING       = 1,
	SPURS_WKL_STATE_RUNNABLE        = 2,
	SPURS_WKL_STATE_SHUTTING_DOWN   = 3,
	SPURS_WKL_STATE_REMOVABLE       = 4,
	SPURS_WKL_STATE_INVALID         = 5,
};

enum SpursImgAddrConstants : u32
{
	// Image addresses
	SPURS_IMG_ADDR_SYS_SRV_WORKLOAD = 0x100,
	SPURS_IMG_ADDR_TASKSET_PM       = 0x200,
};

enum SpursWorkloadGUIDs : u64
{
	// GUID
	SPURS_GUID_SYS_WKL    = 0x1BB841BF38F89D33ull,
	SPURS_GUID_TASKSET_PM = 0x836E915B2E654143ull,
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

	// Task incident
	CELL_SPURS_TRACE_TASK_DISPATCH  = 0x01,
	CELL_SPURS_TRACE_TASK_YIELD     = 0x03,
	CELL_SPURS_TRACE_TASK_WAIT      = 0x04,
	CELL_SPURS_TRACE_TASK_EXIT      = 0x05,

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
	CELL_SPURS_TASKSET_PM_ENTRY_ADDR        = 0xA00,
	CELL_SPURS_TASKSET_PM_SYSCALL_ADDR      = 0xA70,

	// Task syscall numbers
	CELL_SPURS_TASK_SYSCALL_EXIT            = 0,
	CELL_SPURS_TASK_SYSCALL_YIELD           = 1,
	CELL_SPURS_TASK_SYSCALL_WAIT_SIGNAL     = 2,
	CELL_SPURS_TASK_SYSCALL_POLL            = 3,
	CELL_SPURS_TASK_SYSCALL_RECV_WKL_FLAG   = 4,

	// Task poll status
	CELL_SPURS_TASK_POLL_FOUND_TASK         = 1,
	CELL_SPURS_TASK_POLL_FOUND_WORKLOAD     = 2,
};

enum CellSpursEventFlagWaitMode
{
	CELL_SPURS_EVENT_FLAG_OR             = 0,
	CELL_SPURS_EVENT_FLAG_AND            = 1,
	CELL_SPURS_EVENT_FLAG_WAIT_MODE_LAST = CELL_SPURS_EVENT_FLAG_AND,
};

enum CellSpursEventFlagClearMode
{
	CELL_SPURS_EVENT_FLAG_CLEAR_AUTO   = 0,
	CELL_SPURS_EVENT_FLAG_CLEAR_MANUAL = 1,
	CELL_SPURS_EVENT_FLAG_CLEAR_LAST   = CELL_SPURS_EVENT_FLAG_CLEAR_MANUAL,
};

enum CellSpursEventFlagDirection
{
	CELL_SPURS_EVENT_FLAG_SPU2SPU,
	CELL_SPURS_EVENT_FLAG_SPU2PPU,
	CELL_SPURS_EVENT_FLAG_PPU2SPU,
	CELL_SPURS_EVENT_FLAG_ANY2ANY,
	CELL_SPURS_EVENT_FLAG_LAST = CELL_SPURS_EVENT_FLAG_ANY2ANY,
};

enum CellSpursJobOpcode : u64
{
	CELL_SPURS_JOB_OPCODE_NOP          = 0,
	CELL_SPURS_JOB_OPCODE_RESET_PC     = 1,
	CELL_SPURS_JOB_OPCODE_SYNC         = 2 | (0 << 3),
	CELL_SPURS_JOB_OPCODE_LWSYNC       = 2 | (2 << 3),
	CELL_SPURS_JOB_OPCODE_SYNC_LABEL   = 2 | (1 << 3),
	CELL_SPURS_JOB_OPCODE_LWSYNC_LABEL = 2 | (3 << 3),
	CELL_SPURS_JOB_OPCODE_NEXT         = 3,
	CELL_SPURS_JOB_OPCODE_CALL         = 4,
	CELL_SPURS_JOB_OPCODE_FLUSH        = 5,
	CELL_SPURS_JOB_OPCODE_JOBLIST      = 6,
	CELL_SPURS_JOB_OPCODE_ABORT        = 7 | (0 << 3),
	CELL_SPURS_JOB_OPCODE_GUARD        = 7 | (1 << 3),
	CELL_SPURS_JOB_OPCODE_SET_LABEL    = 7 | (2 << 3),
	CELL_SPURS_JOB_OPCODE_RET          = 7 | (14 << 3),
	CELL_SPURS_JOB_OPCODE_END          = 7 | (15 << 3),
	CELL_SPURS_JOB_OPCODE_JTS          = 0x800000000ull | CELL_SPURS_JOB_OPCODE_LWSYNC,
};

enum CellSpursJobChainRevision : u32
{
	CELL_SPURS_JOB_REVISION_0 = 0,
	CELL_SPURS_JOB_REVISION_1 = 1,
	CELL_SPURS_JOB_REVISION_2 = 2,
	CELL_SPURS_JOB_REVISION_3 = 3,
};

// Event flag constants
enum SpursEventFlagConstants
{
	CELL_SPURS_EVENT_FLAG_MAX_WAIT_SLOTS   = 16,
	CELL_SPURS_EVENT_FLAG_INVALID_SPU_PORT = 0xFF,
};

struct alignas(16) CellSpursWorkloadFlag
{
	be_t<u64> unused0;
	be_t<u32> unused1;
	atomic_be_t<u32> flag;
};

CHECK_SIZE_ALIGN(CellSpursWorkloadFlag, 16, 16);

struct CellSpursInfo
{
	be_t<s32> nSpus;                  // 0x00
	be_t<s32> spuThreadGroupPriority; // 0x04
	be_t<s32> ppuThreadPriority;      // 0x08
	b8 exitIfNoWork;                  // 0x0C
	b8 spurs2;                        // 0x0D
	u8 padding24[2];                  // 0x0E
	vm::bptr<void> traceBuffer;       // 0x10
	be_t<u32> padding32;              // 0x14
	be_t<u64> traceBufferSize;        // 0x18
	be_t<u32> traceMode;              // 0x20
	be_t<u32> spuThreadGroup;         // 0x24
	be_t<u32> spuThreads[8];          // 0x28
	be_t<u64> spursHandlerThread0;    // 0x48
	be_t<u64> spursHandlerThread1;    // 0x50
	char namePrefix[16];              // 0x58
	be_t<u32> namePrefixLength;       // 0x68
	be_t<u32> deadlineMissCounter;    // 0x6C
	be_t<u32> deadlineMeetCounter;    // 0x70
	u8 padding[164];                  // 0x74
};

CHECK_SIZE(CellSpursInfo, 280);

struct alignas(8) CellSpursAttribute
{
	be_t<u32> revision;    // 0x0
	be_t<u32> sdkVersion;  // 0x4
	be_t<u32> nSpus;       // 0x8
	be_t<s32> spuPriority; // 0xC
	be_t<s32> ppuPriority; // 0x10
	b8 exitIfNoWork;       // 0x14
	char prefix[15];       // 0x15 (not a NTS)
	be_t<u32> prefixSize;  // 0x24
	be_t<u32> flags;       // 0x28 (SpursAttrFlags)
	be_t<u32> container;   // 0x2C
	be_t<u32> unk0;        // 0x30
	be_t<u32> unk1;        // 0x34
	u8 swlPriority[8];     // 0x38
	be_t<u32> swlMaxSpu;   // 0x40
	be_t<u32> swlIsPreem;  // 0x44
	u8 padding[440];
};

CHECK_SIZE_ALIGN(CellSpursAttribute, 512, 8);

using CellSpursShutdownCompletionEventHook = void(vm::ptr<CellSpurs> spurs, u32 wid, vm::ptr<void> arg);

struct alignas(16) CellSpursTraceInfo
{
	be_t<u32> spuThread[8];     // 0x00
	be_t<u32> count[8];         // 0x20
	be_t<u32> spuThreadGroup;   // 0x40
	be_t<u32> numSpus;          // 0x44
	u8 padding[56];
};

CHECK_SIZE_ALIGN(CellSpursTraceInfo, 128, 16);

struct CellSpursTraceHeader
{
	u8 tag;
	u8 length;
	u8 spu;
	u8 workload;
	be_t<u32> time;
};

struct CellSpursTraceControlData
{
	be_t<u32> incident;
	be_t<u32> reserved;
};

struct CellSpursTraceServiceData
{
	be_t<u32> incident;
	be_t<u32> reserved;
};

struct CellSpursTraceTaskData
{
	be_t<u32> incident;
	be_t<u32> taskId;
};

struct CellSpursTraceJobData
{
	u8 reserved[3];
	u8 binLSAhigh8;
	be_t<u32> jobDescriptor;
};

struct CellSpursTraceLoadData
{
	be_t<u32> ea;
	be_t<u16> ls;
	be_t<u16> size;
};

struct CellSpursTraceMapData
{
	be_t<u32> offset;
	be_t<u16> ls;
	be_t<u16> size;
};

struct CellSpursTraceStartData
{
	char _module[4];
	be_t<u16> level;
	be_t<u16> ls;
};

struct alignas(16) CellSpursTracePacket
{
	CellSpursTraceHeader header;

	union
	{
		CellSpursTraceControlData control;
		CellSpursTraceServiceData service;
		CellSpursTraceTaskData task;
		CellSpursTraceJobData job;

		CellSpursTraceLoadData load;
		CellSpursTraceMapData map;
		CellSpursTraceStartData start;

		be_t<u64> stop;
		be_t<u64> user;
		be_t<u64> guid;
		be_t<u64> raw;
	}
	data;
};

CHECK_SIZE_ALIGN(CellSpursTracePacket, 16, 16);

struct CellSpursJobChain;
struct CellSpursExceptionInfo;

using CellSpursJobChainExceptionEventHandler = void(vm::ptr<CellSpurs> spurs, vm::ptr<CellSpursJobChain> jobChain, vm::cptr<CellSpursExceptionInfo> info
    , vm::cptr<void> eaJobBinary, u32 lsAddrJobBinary, vm::ptr<void> arg);

struct alignas(128) CellSpursJobChain
{
	vm::bcptr<u64, u64> pc;                                                      // 0x00
	vm::bcptr<u64, u64> linkRegister[3];                                         // 0x08
	u8 unk0[0x3];                                                                // 0x20
	b8 isHalted;                                                                 // 0x23
	b8 autoReadyCount;                                                           // 0x24
	u8 unk1[0x3];                                                                // 0x25
	u8 initSpuCount;                                                             // 0x28
	u8 unk5;                                                                     // 0x29
	u8 tag1;                                                                     // 0x2A
	u8 tag2;                                                                     // 0x2B
	u8 val2C;                                                                    // 0x2C
	u8 jmVer;                                                                    // 0x2D
	u8 val2E;                                                                    // 0x2E
	u8 val2F;                                                                    // 0x2F
	atomic_be_t<u64> urgentCmds[4];                                              // 0x30
	u8 unk2[0x20];                                                               // 0x50
	atomic_be_t<u16> maxGrabbedJob;                                              // 0x70
	be_t<u16> sizeJobDescriptor;                                                 // 0x72
	be_t<u32> workloadId;                                                        // 0x74
	vm::bptr<CellSpurs, u64> spurs;                                              // 0x78
	be_t<s32> error;                                                             // 0x80
	be_t<u32> unk3;                                                              // 0x84
	vm::bptr<void, u64> cause;                                                   // 0x88
	be_t<u32> sdkVer;                                                            // 0x90
	u8 jobMemoryCheck;                                                           // 0x94 (unlike Attribute::jobMmeoryCheck it is not a boolean but a bitset it seems)
	u8 unk4[0x3];                                                                // 0x95
	vm::bptr<CellSpursJobChainExceptionEventHandler, u64> exceptionEventHandler; // 0x98
	vm::bptr<void, u64> exceptionEventHandlerArgument;                           // 0xA0
	u8 unk6[0x100 - 0xA8];
};

struct alignas(128) CellSpursJobChain_x00
{
	vm::bcptr<u64, u64> pc;                                                      // 0x00
	vm::bcptr<u64, u64> linkRegister[3];                                         // 0x08
	u8 unk0[0x3];                                                                // 0x20
	b8 isHalted;                                                                 // 0x23
	b8 autoReadyCount;                                                           // 0x24
	u8 unk1[0x3];                                                                // 0x25
	u8 initSpuCount;                                                             // 0x28
	u8 unk5;                                                                     // 0x29
	u8 tag1;                                                                     // 0x2A
	u8 tag2;                                                                     // 0x2B
	u8 val2C;                                                                    // 0x2C
	u8 jmVer;                                                                    // 0x2D
	u8 val2E;                                                                    // 0x2E
	u8 val2F;                                                                    // 0x2F
	be_t<u64> urgentCmds[4];                                                     // 0x30
	u8 unk2[0x20];                                                               // 0x50
	be_t<u16> maxGrabbedJob;                                                     // 0x70
	be_t<u16> sizeJobDescriptor;                                                 // 0x72
	be_t<u32> workloadId;                                                        // 0x74
	vm::bptr<CellSpurs, u64> spurs;                                              // 0x78
};

struct CellSpursJobChainInfo
{
	be_t<u64> urgentCommandSlot[4];                                                 // 0x00
	vm::bcptr<u64> programCounter;                                                  // 0x20
	vm::bcptr<u64> linkRegister[3];                                                 // 0x24
	vm::bcptr<void> cause;                                                          // 0x30
	be_t<u32> statusCode;                                                           // 0x34
	be_t<u32> maxSizeJobDescriptor;                                                 // 0x38
	be_t<u32> idWorkload;                                                           // 0x3C
	b8 autoReadyCount;                                                              // 0x40
	b8 isHalted;                                                                    // 0x41
	b8 isFixedMemAlloc;                                                             // 0x42
	u8 padding8;                                                                    // 0x43
	u16 maxGrabbedJob;                                                              // 0x44
	u16 padding16;                                                                  // 0x46
	vm::bcptr<char> name;                                                           // 0x48
	vm::bptr<CellSpursJobChainExceptionEventHandler> exceptionEventHandler;         // 0x4C
	vm::bptr<void> exceptionEventHandlerArgument;                                   // 0x50
};

struct alignas(8) CellSpursJobChainAttribute
{
	be_t<u32> jmVer;                  // 0x00
	be_t<u32> sdkVer;                 // 0x04
	vm::bcptr<u64> jobChainEntry;     // 0x08
	be_t<u16> sizeJobDescriptor;      // 0x0C
	be_t<u16> maxGrabbedJob;          // 0x0E
	u8 priorities[8];                 // 0x10
	be_t<u32> maxContention;          // 0x18
	b8 autoSpuCount;                  // 0x1C
	u8 padding[3];                    // 0x1D
	be_t<u32> tag1;                   // 0x20
	be_t<u32> tag2;                   // 0x24
	b8 isFixedMemAlloc;               // 0x28
	u8 padding1[3];                   // 0x29
	be_t<u32> maxSizeJobDescriptor;   // 0x2C
	be_t<u32> initSpuCount;           // 0x30
	be_t<u32> haltOnError;            // 0x34
	vm::bcptr<char> name;             // 0x38
	b8 jobMemoryCheck;                // 0x3C
};

struct CellSpursWorkloadInfo
{
	be_t<u64> data;                                     // 0x00
	u8 priority[8];                                     // 0x08
	vm::bcptr<void> policyModule;                       // 0x10
	be_t<u32> sizePolicyModule;                         // 0x14
	vm::bcptr<char> nameClass;                          // 0x18
	vm::bcptr<char> nameInstance;                       // 0x1C
	u8 contention;                                      // 0x20
	u8 minContention;                                   // 0x21
	u8 maxContention;                                   // 0x22
	u8 readyCount;                                      // 0x23
	u8 idleSpuRequest;                                  // 0x24
	u8 hasSignal;                                       // 0x25
	u8 padding[2];                                      // 0x26
	vm::bptr<void> shutdownCompletionEventHook;         // 0x28
	vm::bptr<void> shutdownCompletionEventHookArgument; // 0x2C
};

struct alignas(128) CellSpursJobGuard
{
	atomic_be_t<u32> ncount0;             // 0x00
	be_t<u32> ncount1;                    // 0x04
	vm::bptr<CellSpursJobChain> jobChain; // 0x0C
	be_t<u32> unk0;
	be_t<u32> requestSpuCount;            // 0x10
	be_t<u32> unk1[3];
	be_t<u32> autoReset;                  // 0x20
	be_t<u32> unk2[3];
	be_t<u32> zero;                       // 0x30
	u8 unk3[0x80 - 0x34];
};

CHECK_SIZE_ALIGN(CellSpursJobGuard, 128, 128);

struct alignas(128) CellSpursJobGuard_x00
{
	be_t<u32> ncount0;                    // 0x00
	be_t<u32> ncount1;                    // 0x04
	vm::bptr<CellSpursJobChain> jobChain; // 0x0C
	be_t<u32> unk0;
	be_t<u32> requestSpuCount;            // 0x10
	be_t<u32> unk1[3];
	be_t<u32> autoReset;                  // 0x20
	be_t<u32> unk2[3];
	be_t<u32> zero;                       // 0x30
	u8 unk3[0x80 - 0x34];
};

CHECK_SIZE_ALIGN(CellSpursJobGuard_x00, 128, 128);

// Core CellSpurs structures
struct alignas(128) CellSpurs
{
	struct _sub_str1
	{
		u8 unk0[0x20]; // 0x00 - SPU exception handler 0x08 - SPU exception handler args
		be_t<u64> sem; // 0x20
		atomic_be_t<u32> x28; // 0x28
		be_t<u32> x2C; // 0x2C
		vm::bptr<CellSpursShutdownCompletionEventHook, u64> hook; // 0x30
		vm::bptr<void, u64> hookArg; // 0x38
		u8 unk2[0x40];
	};

	CHECK_SIZE(_sub_str1, 128);

	struct EventPortMux;

	using EventHandlerCallback = void(vm::ptr<EventPortMux>, u64 data);

	struct EventHandlerListNode
	{
		vm::bptr<EventHandlerListNode, u64> next;
		be_t<u64> data;
		vm::bptr<EventHandlerCallback, u64> handler;
	};

	struct EventPortMux
	{
		atomic_be_t<u32> reqPending;  // 0x00
		be_t<u32> spuPort;            // 0x04
		be_t<u32> x08;                // 0x08
		be_t<u32> x0C;                // 0x0C
		be_t<u64> eventPort;          // 0x10
		atomic_t<vm::bptr<EventHandlerListNode, u64>> handlerList; // 0x18
		u8 x20[0x80 - 0x20];          // 0x20
	};

	CHECK_SIZE(EventPortMux, 128);

	struct WorkloadInfo
	{
		vm::bcptr<void, u64> addr;         // 0x00 Address of the executable
		be_t<u64> arg;                     // 0x08 Argument
		be_t<u32> size;                    // 0x10 Size of the executable
		atomic_t<u8> uniqueId;             // 0x14 Unique id of the workload. It is the same for all workloads with the same addr.
		u8 pad[3];

		union
		{
			atomic_t<u64> prio64;
			u8 priority[8];                // 0x18 Priority of the workload on each SPU
		};
	};

	CHECK_SIZE(WorkloadInfo, 32);

	struct _sub_str4
	{
		vm::bcptr<char, u64> nameClass;
		vm::bcptr<char, u64> nameInstance;
	};

	atomic_t<u8> wklReadyCount1[0x10];                  // 0x00 Number of SPUs requested by each workload (0..15 wids).
	atomic_t<u8> wklIdleSpuCountOrReadyCount2[0x10];    // 0x10 SPURS1: Number of idle SPUs requested by each workload (0..15 wids). SPURS2: Number of SPUs requested by each workload (16..31 wids).
	u8 wklCurrentContention[0x10];                      // 0x20 Number of SPUs used by each workload. SPURS1: index = wid. SPURS2: packed 4-bit data, index = wid % 16, internal index = wid / 16.
	u8 wklPendingContention[0x10];                      // 0x30 Number of SPUs that are pending to context switch to the workload. SPURS1: index = wid. SPURS2: packed 4-bit data, index = wid % 16, internal index = wid / 16.
	u8 wklMinContention[0x10];                          // 0x40 Min SPUs required for each workload. SPURS1: index = wid. SPURS2: Unused.
	atomic_t<u8> wklMaxContention[0x10];                // 0x50 Max SPUs that may be allocated to each workload. SPURS1: index = wid. SPURS2: packed 4-bit data, index = wid % 16, internal index = wid / 16.
	CellSpursWorkloadFlag wklFlag;                      // 0x60
	atomic_be_t<u16> wklSignal1;                        // 0x70 Bitset for 0..15 wids
	atomic_t<u8> sysSrvMessage;                         // 0x72
	u8 spuIdling;                                       // 0x73
	u8 flags1;                                          // 0x74 Type is SpursFlags1
	u8 sysSrvTraceControl;                              // 0x75
	u8 nSpus;                                           // 0x76
	atomic_t<u8> wklFlagReceiver;                       // 0x77
	atomic_be_t<u16> wklSignal2;                        // 0x78 Bitset for 16..32 wids
	u8 x7A[6];                                          // 0x7A
	atomic_t<SpursWorkloadState> wklState1[0x10];       // 0x80
	u8 wklStatus1[0x10];                                // 0x90
	atomic_t<u8> wklEvent1[0x10];                       // 0xA0
	atomic_be_t<u32> wklEnabled;                        // 0xB0
	atomic_be_t<u32> wklMskB;                           // 0xB4 - System service - Available module id
	u32 xB8;                                            // 0xB8
	u8 sysSrvExitBarrier;                               // 0xBC
	atomic_t<u8> sysSrvMsgUpdateWorkload;               // 0xBD
	u8 xBE;                                             // 0xBE
	u8 sysSrvMsgTerminate;                              // 0xBF
	u8 sysSrvPreemptWklId[8];                           // 0xC0 Id of the workload that was preempted by the system workload on each SPU
	u8 sysSrvOnSpu;                                     // 0xC8
	u8 spuPort;                                         // 0xC9
	u8 xCA;                                             // 0xCA
	u8 xCB;                                             // 0xCB

	struct SrvTraceSyncVar
	{
		u8 sysSrvTraceInitialised;                      // 0xCC
		u8 sysSrvNotifyUpdateTraceComplete;             // 0xCD
		u8 sysSrvMsgUpdateTrace;                        // 0xCE
		u8 xCF;
	};

	atomic_t<SrvTraceSyncVar> sysSrvTrace;              // 0xCC

	atomic_t<SpursWorkloadState> wklState2[0x10];       // 0xD0
	u8 wklStatus2[0x10];                                // 0xE0
	atomic_t<u8> wklEvent2[0x10];                       // 0xF0
	_sub_str1 wklF1[0x10];                              // 0x100
	vm::bptr<CellSpursTraceInfo, u64> traceBuffer;      // 0x900
	be_t<u32> traceStartIndex[6];                       // 0x908
	u8 unknown7[0x948 - 0x920];                         // 0x920
	be_t<u64> traceDataSize;                            // 0x948
	be_t<u32> traceMode;                                // 0x950
	u8 unknown8[0x980 - 0x954];                         // 0x954
	be_t<u64> semPrv;                                   // 0x980
	be_t<u32> unk11;                                    // 0x988
	be_t<u32> unk12;                                    // 0x98C
	be_t<u64> unk13;                                    // 0x990
	u8 unknown4[0xB00 - 0x998];
	WorkloadInfo wklInfo1[0x10];                        // 0xB00
	WorkloadInfo wklInfoSysSrv;                         // 0xD00
	be_t<u64> ppu0;                                     // 0xD20 Handler thread
	be_t<u64> ppu1;                                     // 0xD28
	be_t<u32> spuTG;                                    // 0xD30 SPU thread group
	be_t<u32> spus[8];                                  // 0xD34
	u8 unknown3[0xD5C - 0xD54];
	be_t<u32> eventQueue;                               // 0xD5C
	be_t<u32> eventPort;                                // 0xD60
	atomic_t<u8> handlerDirty;                          // 0xD64
	atomic_t<u8> handlerWaiting;                        // 0xD65
	atomic_t<u8> handlerExiting;                        // 0xD66
	u8 xD67;                                            // 0xD67
	atomic_be_t<u32> enableEH;                          // 0xD68
	be_t<u32> exception;                                // 0xD6C
	sys_spu_image spuImg;                               // 0xD70
	be_t<u32> flags;                                    // 0xD80
	be_t<s32> spuPriority;                              // 0xD84
	be_t<u32> ppuPriority;                              // 0xD88
	char prefix[0x0f];                                  // 0xD8C
	u8 prefixSize;                                      // 0xD9B
	be_t<u32> unk5;                                     // 0xD9C
	be_t<u32> revision;                                 // 0xDA0
	be_t<u32> sdkVersion;                               // 0xDA4
	atomic_be_t<u64> spuPortBits;                       // 0xDA8
	sys_lwmutex_t mutex;                                // 0xDB0
	sys_lwcond_t cond;                                  // 0xDC8
	u8 unknown9[0xE00 - 0xDD0];
	_sub_str4 wklH1[0x10];                              // 0xE00
	EventPortMux eventPortMux;                          // 0xF00
	atomic_be_t<u64> globalSpuExceptionHandler;         // 0xF80
	be_t<u64> globalSpuExceptionHandlerArgs;            // 0xF88
	u8 unknown6[0x1000 - 0xF90];
	WorkloadInfo wklInfo2[0x10];                        // 0x1000
	_sub_str1 wklF2[0x10];                              // 0x1200
	_sub_str4 wklH2[0x10];                              // 0x1A00
	u8 unknown_[0x2000 - 0x1B00];

	u32 max_workloads() const
	{
		return (flags1 & SF1_32_WORKLOADS ? CELL_SPURS_MAX_WORKLOAD2 : CELL_SPURS_MAX_WORKLOAD);
	}

	atomic_t<SpursWorkloadState>& wklState(u32 wid)
	{
		if (wid & 0x10)
		{
			return wklState2[wid & 0xf];
		}
		else
		{
			return wklState1[wid & 0xf];
		}
	}

	u8 wklStatus(u32 wid) const
	{
		if (wid & 0x10)
		{
			return atomic_storage<u8>::load(wklStatus2[wid & 0xf]);
		}
		else
		{
			return atomic_storage<u8>::load(wklStatus1[wid & 0xf]);
		}
	}

	atomic_t<u8>& wklEvent(u32 wid)
	{
		if (wid & 0x10)
		{
			return wklEvent2[wid & 0xf];
		}
		else
		{
			return wklEvent1[wid & 0xf];
		}
	}

	atomic_t<u8>& readyCount(u32 wid)
	{
		if (wid < 0x10)
		{
			return wklReadyCount1[wid & 0xf];
		}
		else
		{
			return wklIdleSpuCountOrReadyCount2[wid & 0xf];
		}
	}

	_sub_str1& wklSyncInfo(u32 wid)
	{
		if (wid & 0x10)
		{
			return wklF2[wid & 0xf];
		}
		else
		{
			return wklF1[wid & 0xf];
		}
	}

	_sub_str4& wklName(u32 wid)
	{
		if (wid & 0x10)
		{
			return wklH2[wid & 0xf];
		}
		else
		{
			return wklH1[wid & 0xf];
		}
	}

	WorkloadInfo& wklInfo(u32 wid)
	{
		if (wid & 0x10)
		{
			return wklInfo2[wid & 0xf];
		}
		else
		{
			return wklInfo1[wid & 0xf];
		}
	}
};

CHECK_SIZE_ALIGN(CellSpurs, 0x2000, 128);

using CellSpurs2 = CellSpurs;

struct CellSpursExceptionInfo
{
	be_t<u32> spu_thread;
	be_t<u32> spu_npc;
	be_t<u32> cause;
	be_t<u64> option;
};

// Exception handler
using CellSpursGlobalExceptionEventHandler = void(vm::ptr<CellSpurs> spurs, vm::cptr<CellSpursExceptionInfo> info, u32 id, vm::ptr<void> arg);

struct CellSpursWorkloadAttribute
{
	be_t<u32> revision;
	be_t<u32> sdkVersion;
	vm::bcptr<void> pm;
	be_t<u32> size;
	be_t<u64> data;
	u8 priority[8];
	be_t<u32> minContention;
	be_t<u32> maxContention;
	vm::bcptr<char> nameClass;
	vm::bcptr<char> nameInstance;
	vm::bptr<CellSpursShutdownCompletionEventHook> hook;
	vm::bptr<void> hookArg;
	u8 padding[456];
};

CHECK_SIZE_ALIGN(CellSpursWorkloadAttribute, 512, 8);

struct alignas(128) CellSpursEventFlag
{
	struct ControlSyncVar
	{
		be_t<u16> events;                // 0x00 Event bits
		be_t<u16> spuTaskPendingRecv;    // 0x02 A bit is set to 1 when the condition of the SPU task using the slot are met and back to 0 when the SPU task unblocks
		be_t<u16> ppuWaitMask;           // 0x04 Wait mask for blocked PPU thread
		u8 ppuWaitSlotAndMode;           // 0x06 Top 4 bits: Wait slot number of the blocked PPU threads, Bottom 4 bits: Wait mode of the blocked PPU thread
		u8 ppuPendingRecv;               // 0x07 Set to 1 when the blocked PPU thread's conditions are met and back to 0 when the PPU thread is unblocked
	};

	union
	{
		atomic_t<ControlSyncVar> ctrl;    // 0x00
		atomic_be_t<u16> events;          // 0x00
	};

	be_t<u16> spuTaskUsedWaitSlots;      // 0x08 A bit is set to 1 if the wait slot corresponding to the bit is used by an SPU task and 0 otherwise
	be_t<u16> spuTaskWaitMode;           // 0x0A A bit is set to 1 if the wait mode for the SPU task corresponding to the bit is AND and 0 otherwise
	u8 spuPort;                          // 0x0C
	u8 isIwl;                            // 0x0D
	u8 direction;                        // 0x0E
	u8 clearMode;                        // 0x0F
	be_t<u16> spuTaskWaitMask[16];       // 0x10 Wait mask for blocked SPU tasks
	be_t<u16> pendingRecvTaskEvents[16]; // 0x30 The value of event flag when the wait condition for the thread/task was met
	u8 waitingTaskId[16];                // 0x50 Task id of waiting SPU threads
	u8 waitingTaskWklId[16];             // 0x60 Workload id of waiting SPU threads
	be_t<u64> addr;                      // 0x70
	be_t<u32> eventPortId;               // 0x78
	be_t<u32> eventQueueId;              // 0x7C
};

CHECK_SIZE_ALIGN(CellSpursEventFlag, 128, 128);

struct alignas(128) CellSpursEventFlag_x00
{
	struct alignas(8) ControlSyncVar
	{
		be_t<u16> events;                // 0x00 Event bits
		be_t<u16> spuTaskPendingRecv;    // 0x02 A bit is set to 1 when the condition of the SPU task using the slot are met and back to 0 when the SPU task unblocks
		be_t<u16> ppuWaitMask;           // 0x04 Wait mask for blocked PPU thread
		u8 ppuWaitSlotAndMode;           // 0x06 Top 4 bits: Wait slot number of the blocked PPU threads, Bottom 4 bits: Wait mode of the blocked PPU thread
		u8 ppuPendingRecv;               // 0x07 Set to 1 when the blocked PPU thread's conditions are met and back to 0 when the PPU thread is unblocked
	};

	ControlSyncVar ctrl;                 // 0x00
	be_t<u16> spuTaskUsedWaitSlots;      // 0x08 A bit is set to 1 if the wait slot corresponding to the bit is used by an SPU task and 0 otherwise
	be_t<u16> spuTaskWaitMode;           // 0x0A A bit is set to 1 if the wait mode for the SPU task corresponding to the bit is AND and 0 otherwise
	u8 spuPort;                          // 0x0C
	u8 isIwl;                            // 0x0D
	u8 direction;                        // 0x0E
	u8 clearMode;                        // 0x0F
	be_t<u16> spuTaskWaitMask[16];       // 0x10 Wait mask for blocked SPU tasks
	be_t<u16> pendingRecvTaskEvents[16]; // 0x30 The value of event flag when the wait condition for the thread/task was met
	u8 waitingTaskId[16];                // 0x50 Task id of waiting SPU threads
	u8 waitingTaskWklId[16];             // 0x60 Workload id of waiting SPU threads
	be_t<u64> addr;                      // 0x70
	be_t<u32> eventPortId;               // 0x78
	be_t<u32> eventQueueId;              // 0x7C
};

using CellSpursLFQueue = CellSyncLFQueue;

union CellSpursTaskArgument
{
	be_t<u32> _u32[4];
	be_t<u64> _u64[2];
};

union CellSpursTaskLsPattern
{
	be_t<u32> _u32[4];
	be_t<u64> _u64[2];
};

struct alignas(16) CellSpursTaskAttribute
{
	u8 reserved[256];
};

CHECK_SIZE_ALIGN(CellSpursTaskAttribute, 256, 16);

struct alignas(16) CellSpursTaskAttribute2
{
	be_t<u32> revision;
	be_t<u32> sizeContext;
	be_t<u64> eaContext;
	CellSpursTaskLsPattern lsPattern;
	vm::bcptr<char> name;

	u8 reserved[220];
};

CHECK_SIZE_ALIGN(CellSpursTaskAttribute2, 256, 16);

// Exception handler
using CellSpursTasksetExceptionEventHandler = void(vm::ptr<CellSpurs> spurs, vm::ptr<CellSpursTaskset> taskset, u32 idTask, vm::cptr<CellSpursExceptionInfo> info, vm::ptr<void> arg);

struct alignas(128) CellSpursTaskExitCode
{
	u8 skip[128];
};

CHECK_SIZE_ALIGN(CellSpursTaskExitCode, 128, 128);

struct CellSpursTaskInfo
{
	CellSpursTaskLsPattern lsPattern;
	CellSpursTaskArgument argument;
	vm::bptr<void> eaElf;
	vm::bptr<void> eaContext;
	be_t<u32> sizeContext;
	u8 state;
	u8 hasSignal;
	u8 padding[2];
	vm::bcptr<CellSpursTaskExitCode> eaTaskExitCode;
	u8 guid[8];
	u8 reserved[12];
};

CHECK_SIZE(CellSpursTaskInfo, 72);

struct CellSpursTasksetInfo
{
	CellSpursTaskInfo taskInfo[CELL_SPURS_MAX_TASK];
	be_t<u64> argument;
	be_t<u32> idWorkload;
	be_t<u32> idLastScheduledTask;
	vm::bcptr<char> name;
	vm::bptr<CellSpursTasksetExceptionEventHandler> exceptionEventHandler;
	vm::bptr<void> exceptionEventHandlerArgument;
	be_t<u32> sizeTaskset;
	u8 reserved[112];
};

CHECK_SIZE(CellSpursTasksetInfo, 9360);

struct alignas(8) CellSpursTasksetAttribute
{
	be_t<u32> revision;             // 0x00
	be_t<u32> sdk_version;          // 0x04
	be_t<u64> args;                 // 0x08
	u8 priority[8];                 // 0x10
	be_t<u32> max_contention;       // 0x18
	vm::bcptr<char> name;           // 0x1C
	be_t<u32> taskset_size;         // 0x20
	be_t<s32> enable_clear_ls;      // 0x24
	u8 reserved[472];
};

CHECK_SIZE_ALIGN(CellSpursTasksetAttribute, 512, 8);

struct alignas(128) CellSpursTaskset
{
	struct TaskInfo
	{
		CellSpursTaskArgument args;                         // 0x00
		vm::bcptr<void, u64> elf;                           // 0x10
		be_t<u64> context_save_storage_and_alloc_ls_blocks; // 0x18 This is (context_save_storage_addr | allocated_ls_blocks)
		CellSpursTaskLsPattern ls_pattern;                  // 0x20
	};

	CHECK_SIZE(TaskInfo, 48);

	struct atomic_tasks_bitset
	{
		atomic_be_t<u32> values[4];

		u32 get_bit(u32 bit) const
		{
			return values[bit / 32] & ((1u << 31) >> (bit % 32));
		}
	};

	atomic_tasks_bitset running;                 // 0x00
	atomic_tasks_bitset ready;                   // 0x10
	atomic_tasks_bitset pending_ready;           // 0x20
	atomic_tasks_bitset enabled;                 // 0x30
	atomic_tasks_bitset signalled;               // 0x40
	atomic_tasks_bitset waiting;                 // 0x50
	vm::bptr<CellSpurs, u64> spurs;              // 0x60
	be_t<u64> args;                              // 0x68
	u8 enable_clear_ls;                          // 0x70
	u8 x71;                                      // 0x71
	u8 wkl_flag_wait_task;                       // 0x72
	u8 last_scheduled_task;                      // 0x73
	be_t<u32> wid;                               // 0x74
	be_t<u64> x78;                               // 0x78
	TaskInfo task_info[128];                     // 0x80
	vm::bptr<CellSpursTasksetExceptionEventHandler, u64> exception_handler; // 0x1880
	vm::bptr<u64, u64> exception_handler_arg;    // 0x1888
	be_t<u32> size;                              // 0x1890
	u32 unk2;                                    // 0x1894
	u32 event_flag_id1;                          // 0x1898
	u32 event_flag_id2;                          // 0x189C
	u8 unk3[0x60];                               // 0x18A0
};

CHECK_SIZE_ALIGN(CellSpursTaskset, 128 * 50, 128);

struct alignas(128) spurs_taskset_signal_op
{
	be_t<u32> running[4];            // 0x00
	be_t<u32> ready[4];              // 0x10
	be_t<u32> pending_ready[4];      // 0x20
	be_t<u32> enabled[4];            // 0x30
	be_t<u32> signalled[4];          // 0x40
	be_t<u32> waiting[4];            // 0x50
	vm::bptr<CellSpurs, u64> spurs;  // 0x60
	be_t<u64> args;                  // 0x68
	u8 enable_clear_ls;              // 0x70
	u8 x71;                          // 0x71
	u8 wkl_flag_wait_task;           // 0x72
	u8 last_scheduled_task;          // 0x73
	be_t<u32> wid;                   // 0x74
	be_t<u64> x78;                   // 0x78
};

struct alignas(128) spurs_wkl_state_op
{
	SpursWorkloadState wklState1[0x10]; // 0x00
	u8 wklStatus1[0x10];                // 0x10
	u8 wklEvent1[0x10];                 // 0x20
	be_t<u32> wklEnabled;               // 0x30
	be_t<u32> wklMskB;                  // 0x34
	u8 uns[0x5];                        // 0x38
	u8 sysSrvMsgUpdateWorkload;         // 0x3D
	u8 uns2[0x12];                      // 0x3E
	SpursWorkloadState wklState2[0x10]; // 0x50
	u8 wklStatus2[0x10];                // 0x60
	u8 wklEvent2[0x10];                 // 0x70
};

struct alignas(128) CellSpursTaskset2
{
	struct TaskInfo
	{
		CellSpursTaskArgument args;
		vm::bptr<u64, u64> elf_addr;
		vm::bptr<u64, u64> context_save_storage; // This is (context_save_storage_addr | allocated_ls_blocks)
		CellSpursTaskLsPattern ls_pattern;
	};

	CHECK_SIZE(TaskInfo, 48);

	be_t<u32> running_set[4];                    // 0x00
	be_t<u32> ready_set[4];                      // 0x10
	be_t<u32> ready2_set[4];                     // 0x20 - TODO: Find out what this is
	be_t<u32> enabled_set[4];                    // 0x30
	be_t<u32> signal_received_set[4];            // 0x40
	be_t<u32> waiting_set[4];                    // 0x50
	vm::bptr<CellSpurs, u64> spurs;              // 0x60
	be_t<u64> args;                              // 0x68
	u8 enable_clear_ls;                          // 0x70
	u8 x71;                                      // 0x71
	u8 x72;                                      // 0x72
	u8 last_scheduled_task;                      // 0x73
	be_t<u32> wid;                               // 0x74
	be_t<u64> x78;                               // 0x78
	TaskInfo task_info[128];                     // 0x80
	vm::bptr<u64, u64> exception_handler;        // 0x1880
	vm::bptr<u64, u64> exception_handler_arg;    // 0x1888
	be_t<u32> size;                              // 0x1890
	u32 unk2;                                    // 0x1894
	u32 event_flag_id1;                          // 0x1898
	u32 event_flag_id2;                          // 0x189C
	u8 unk3[0x1980 - 0x18A0];                    // 0x18A0
	be_t<v128> task_exit_code[128];              // 0x1980
	u8 unk4[0x2900 - 0x2180];                    // 0x2180
};

CHECK_SIZE_ALIGN(CellSpursTaskset2, 128 * 82, 128);

struct alignas(16) CellSpursTaskNameBuffer
{
	char taskName[CELL_SPURS_MAX_TASK][CELL_SPURS_MAX_TASK_NAME_LENGTH];
};

struct alignas(8) CellSpursTasksetAttribute2
{
	be_t<u32> revision;                                 // 0x00
	vm::bcptr<char> name;                               // 0x04
	be_t<u64> args;                                     // 0x08
	u8 priority[8];                                     // 0x10
	be_t<u32> max_contention;                           // 0x18
	be_t<s32> enable_clear_ls;                          // 0x1C
	vm::bptr<CellSpursTaskNameBuffer> task_name_buffer; // 0x20
	u8 reserved[472];
};

CHECK_SIZE_ALIGN(CellSpursTasksetAttribute2, 512, 8);

struct alignas(16) CellSpursTaskBinInfo
{
	be_t<u64> eaElf;
	be_t<u32> sizeContext;
	be_t<u32> reserved;
	CellSpursTaskLsPattern lsPattern;
};

struct alignas(128) CellSpursBarrier
{
	be_t<u32> zero;                     // 0x00
	be_t<u32> remained;                 // 0x04
	u8 unk0[0x34 - 0x8];
	vm::bptr<CellSpursTaskset> taskset; // 0x34
	u8 unk1[0x80 - 0x38];
};

CHECK_SIZE_ALIGN(CellSpursBarrier, 128, 128);

// The SPURS kernel context. This resides at 0x100 of the LS.
struct SpursKernelContext
{
	u8 tempArea[0x80];                              // 0x100
	u8 wklLocContention[0x10];                      // 0x180
	u8 wklLocPendingContention[0x10];               // 0x190
	u8 priority[0x10];                              // 0x1A0
	u8 x1B0[0x10];                                  // 0x1B0
	vm::bptr<CellSpurs, u64> spurs;                 // 0x1C0
	be_t<u32> spuNum;                               // 0x1C8
	be_t<u32> dmaTagId;                             // 0x1CC
	vm::bcptr<void, u64> wklCurrentAddr;            // 0x1D0
	be_t<u32> wklCurrentUniqueId;                   // 0x1D8
	be_t<u32> wklCurrentId;                         // 0x1DC
	be_t<u32> exitToKernelAddr;                     // 0x1E0
	be_t<u32> selectWorkloadAddr;                   // 0x1E4
	u8 moduleId[2];                                 // 0x1E8
	u8 sysSrvInitialised;                           // 0x1EA
	u8 spuIdling;                                   // 0x1EB
	be_t<u16> wklRunnable1;                         // 0x1EC
	be_t<u16> wklRunnable2;                         // 0x1EE
	be_t<u32> x1F0;                                 // 0x1F0
	be_t<u32> x1F4;                                 // 0x1F4
	be_t<u32> x1F8;                                 // 0x1F8
	be_t<u32> x1FC;                                 // 0x1FC
	be_t<u32> x200;                                 // 0x200
	be_t<u32> x204;                                 // 0x204
	be_t<u32> x208;                                 // 0x208
	be_t<u32> x20C;                                 // 0x20C
	be_t<u64> traceBuffer;                          // 0x210
	be_t<u32> traceMsgCount;                        // 0x218
	be_t<u32> traceMaxCount;                        // 0x21C
	u8 wklUniqueId[0x10];                           // 0x220
	u8 x230[0x280 - 0x230];                         // 0x230
	be_t<u32> guid[4];                              // 0x280
};

CHECK_SIZE(SpursKernelContext, 0x190);

// The SPURS taskset policy module context. This resides at 0x2700 of the LS.
struct SpursTasksetContext
{
	u8 tempAreaTaskset[0x80];                       // 0x2700
	u8 tempAreaTaskInfo[0x30];                      // 0x2780
	be_t<u64> x27B0;                                // 0x27B0
	vm::bptr<CellSpursTaskset, u64> taskset;        // 0x27B8
	be_t<u32> kernelMgmtAddr;                       // 0x27C0
	be_t<u32> syscallAddr;                          // 0x27C4
	be_t<u32> x27C8;                                // 0x27C8
	be_t<u32> spuNum;                               // 0x27CC
	be_t<u32> dmaTagId;                             // 0x27D0
	be_t<u32> taskId;                               // 0x27D4
	u8 x27D8[0x2840 - 0x27D8];                      // 0x27D8
	u8 moduleId[16];                                // 0x2840
	u8 stackArea[0x2C80 - 0x2850];                  // 0x2850
	be_t<v128> savedContextLr;                      // 0x2C80
	be_t<v128> savedContextSp;                      // 0x2C90
	be_t<v128> savedContextR80ToR127[48];           // 0x2CA0
	be_t<v128> savedContextFpscr;                   // 0x2FA0
	be_t<u32> savedWriteTagGroupQueryMask;          // 0x2FB0
	be_t<u32> savedSpuWriteEventMask;               // 0x2FB4
	be_t<u32> tasksetMgmtAddr;                      // 0x2FB8
	be_t<u32> guidAddr;                             // 0x2FBC
	be_t<u64> x2FC0;                                // 0x2FC0
	be_t<u64> x2FC8;                                // 0x2FC8
	be_t<u32> taskExitCode;                         // 0x2FD0
	be_t<u32> x2FD4;                                // 0x2FD4
	u8 x2FD8[0x3000 - 0x2FD8];                      // 0x2FD8
};

CHECK_SIZE(SpursTasksetContext, 0x900);

struct SpursJobChainContext
{
	u8 tempAreaJobChain[0x80];                      // 0x4A00
	u8 unk0[3];                                     // 0x4A80
	b8 unkFlag0;                                    // 0x4A83
	u8 unk1[0x10];                                  // 0x4A84
	vm::bptr<CellSpursJobChain> jobChain;           // 0x4A94

	// TODO
};

class SpursModuleExit
{
};
