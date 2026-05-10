#pragma once

#include "Emu/Memory/vm_ptr.h"

// Return Codes
enum CellFiberError : u32
{
	CELL_FIBER_ERROR_AGAIN        = 0x80760001, // Resource was temporarily unavailable
	CELL_FIBER_ERROR_INVAL        = 0x80760002, // Value of the argument is invalid
	CELL_FIBER_ERROR_NOMEM        = 0x80760004, // Insufficient memory
	CELL_FIBER_ERROR_DEADLK       = 0x80760008, // Execution of the operation will cause a deadlock
	CELL_FIBER_ERROR_PERM         = 0x80760009, // Executed operation is not permitted
	CELL_FIBER_ERROR_BUSY         = 0x8076000A, // The operation target is busy
	CELL_FIBER_ERROR_ABORT        = 0x8076000C, // The operation has been aborted
	CELL_FIBER_ERROR_STAT         = 0x8076000F, // State of the operation target is invalid
	CELL_FIBER_ERROR_ALIGN        = 0x80760010, // The alignment of the argument address is invalid
	CELL_FIBER_ERROR_NULL_POINTER = 0x80760011, // Invalid NULL pointer is specified for the argument
	CELL_FIBER_ERROR_NOSYSINIT    = 0x80760020, // cellFiberPpuInitialize() has not been called
};

//
// CellFiberPpuScheduler
//

struct alignas(128) CellFiberPpuScheduler
{
	u8 skip[512];
};

CHECK_SIZE_ALIGN(CellFiberPpuScheduler, 512, 128);

struct alignas(8) CellFiberPpuSchedulerAttribute
{
	u8 privateHeader[16];
	b8 autoCheckFlags;
	b8 debuggerSupport;
	u8 padding[2];
	be_t<u32> autoCheckFlagsIntervalUsec;
	u8 skip[232];
};

CHECK_SIZE_ALIGN(CellFiberPpuSchedulerAttribute, 256, 8);

//
// CellFiberPpu
//

struct alignas(128) CellFiberPpu
{
	u8 skip[896];
};

CHECK_SIZE_ALIGN(CellFiberPpu, 896, 128);

typedef s32(CellFiberPpuEntry)(u64 arg);
typedef void(CellFiberPpuOnExitCallback)(u64 arg, s32 exitCode);

struct alignas(8) CellFiberPpuAttribute
{
	u8 privateHeader[16];
	char name[32];
	vm::bptr<CellFiberPpuOnExitCallback> onExitCallback;
	be_t<u32> __reserved0__;
	be_t<u64> onExitCallbackArg;
	be_t<u64> __reserved1__;
	u8 skip[184];
};

CHECK_SIZE_ALIGN(CellFiberPpuAttribute, 256, 8);

//
// CellFiberPpuContext
//

struct alignas(16) CellFiberPpuContext
{
	u8 skip[640];
};

CHECK_SIZE_ALIGN(CellFiberPpuContext, 640, 16);

typedef void(CellFiberPpuContextEntry)(u64 arg, vm::ptr<CellFiberPpuContext> fiberFrom);

struct alignas(8) CellFiberPpuContextAttribute
{
	u8 privateHeader[16];
	char name[32];
	b8 debuggerSupport;
	u8 skip[79];
};

CHECK_SIZE_ALIGN(CellFiberPpuContextAttribute, 128, 8);

struct CellFiberPpuContextExecutionOption;

typedef vm::ptr<CellFiberPpuContext>(CellFiberPpuSchedulerCallback)(u64 arg0, u64 arg1);

//
// CellFiberPpuUtilWorkerControl
//

struct alignas(128) CellFiberPpuUtilWorkerControl
{
	u8 skip[768];
};

CHECK_SIZE_ALIGN(CellFiberPpuUtilWorkerControl, 768, 128);

struct alignas(8) CellFiberPpuUtilWorkerControlAttribute
{
	CellFiberPpuSchedulerAttribute scheduler;
	be_t<u64> privateHeader[2];
	u8 __reserved__[112];
};

CHECK_SIZE_ALIGN(CellFiberPpuUtilWorkerControlAttribute, 384, 8);
