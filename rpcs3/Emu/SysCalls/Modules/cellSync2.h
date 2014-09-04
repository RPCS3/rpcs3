#pragma once

// Return Codes
enum
{
	CELL_SYNC2_ERROR_AGAIN = 0x80410C01,
	CELL_SYNC2_ERROR_INVAL = 0x80410C02,
	CELL_SYNC2_ERROR_NOMEM = 0x80410C04,
	CELL_SYNC2_ERROR_DEADLK = 0x80410C08,
	CELL_SYNC2_ERROR_PERM = 0x80410C09,
	CELL_SYNC2_ERROR_BUSY = 0x80410C0A,
	CELL_SYNC2_ERROR_STAT = 0x80410C0F,
	CELL_SYNC2_ERROR_ALIGN = 0x80410C10,
	CELL_SYNC2_ERROR_NULL_POINTER = 0x80410C11,
	CELL_SYNC2_ERROR_NOT_SUPPORTED_THREAD = 0x80410C12,
	CELL_SYNC2_ERROR_NO_NOTIFIER = 0x80410C13,
	CELL_SYNC2_ERROR_NO_SPU_CONTEXT_STORAGE = 0x80410C14,
};

// Constants
enum
{
	CELL_SYNC2_NAME_MAX_LENGTH = 31,
	CELL_SYNC2_THREAD_TYPE_PPU_THREAD = 1 << 0,
	CELL_SYNC2_THREAD_TYPE_PPU_FIBER = 1 << 1,
	CELL_SYNC2_THREAD_TYPE_SPURS_TASK = 1 << 2,
	CELL_SYNC2_THREAD_TYPE_SPURS_JOBQUEUE_JOB = 1 << 3,
	CELL_SYNC2_THREAD_TYPE_SPURS_JOB = 1 << 8,
};

struct CellSync2MutexAttribute
{
	be_t<u16> threadTypes;
	be_t<u16> maxWaiters;
	bool recursive;
	char name[CELL_SYNC2_NAME_MAX_LENGTH + 1];
	u8 reserved[];
};

struct CellSync2CondAttribute
{
	be_t<u16> maxWaiters;
	char name[CELL_SYNC2_NAME_MAX_LENGTH + 1];
	u8 reserved[];
};

struct CellSync2SemaphoreAttribute
{
	be_t<u16> threadTypes;
	be_t<u16> maxWaiters;
	char name[CELL_SYNC2_NAME_MAX_LENGTH + 1];
	u8 reserved[];
};

struct CellSync2QueueAttribute
{
	be_t<u32> threadTypes;
	be_t<u64> elementSize;
	be_t<u32> depth;
	be_t<u16> maxPushWaiters;
	be_t<u16> maxPopWaiters;
	char name[CELL_SYNC2_NAME_MAX_LENGTH + 1];
	u8 reserved[];
};