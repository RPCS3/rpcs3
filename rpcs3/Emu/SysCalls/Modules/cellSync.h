#pragma once

// Return Codes
enum
{
	CELL_SYNC_ERROR_AGAIN  = 0x80410101,
	CELL_SYNC_ERROR_INVAL  = 0x80410102,
	CELL_SYNC_ERROR_NOSYS  = 0x80410103, // ???
	CELL_SYNC_ERROR_NOMEM  = 0x80410104,
	CELL_SYNC_ERROR_SRCH   = 0x80410105, // ???
	CELL_SYNC_ERROR_NOENT  = 0x80410106, // ???
	CELL_SYNC_ERROR_NOEXEC = 0x80410107, // ???
	CELL_SYNC_ERROR_DEADLK = 0x80410108,
	CELL_SYNC_ERROR_PERM   = 0x80410109,
	CELL_SYNC_ERROR_BUSY   = 0x8041010A,
	//////////////////////// 0x8041010B, // ???
	CELL_SYNC_ERROR_ABORT  = 0x8041010C, // ???
	CELL_SYNC_ERROR_FAULT  = 0x8041010D, // ???
	CELL_SYNC_ERROR_CHILD  = 0x8041010E, // ???
	CELL_SYNC_ERROR_STAT   = 0x8041010F,
	CELL_SYNC_ERROR_ALIGN  = 0x80410110,

	CELL_SYNC_ERROR_NULL_POINTER           = 0x80410111,

	CELL_SYNC_ERROR_NOT_SUPPORTED_THREAD   = 0x80410112, // ???
	CELL_SYNC_ERROR_SHOTAGE                = 0x80410112, // ???
	CELL_SYNC_ERROR_NO_NOTIFIER            = 0x80410113, // ???
	CELL_SYNC_ERROR_UNKNOWNKEY             = 0x80410113, // ???
	CELL_SYNC_ERROR_NO_SPU_CONTEXT_STORAGE = 0x80410114, // ???
};

struct CellSyncMutex
{
	be_t<u16> m_freed;
	be_t<u16> m_order;

	volatile u32& m_data()
	{
		return *reinterpret_cast<u32*>(this);
	};
};

static_assert(sizeof(CellSyncMutex) == 4, "CellSyncMutex: wrong size");

struct CellSyncBarrier
{
	be_t<s16> m_value;
	be_t<s16> m_count;

	volatile u32& m_data()
	{
		return *reinterpret_cast<u32*>(this);
	};
};

static_assert(sizeof(CellSyncBarrier) == 4, "CellSyncBarrier: wrong size");

struct CellSyncRwm
{
	be_t<u16> m_readers;
	be_t<u16> m_writers;
	be_t<u32> m_size;
	be_t<u64> m_addr;

	volatile u32& m_data()
	{
		return *reinterpret_cast<u32*>(this);
	};
};

static_assert(sizeof(CellSyncRwm) == 16, "CellSyncBarrier: wrong size");

struct CellSyncQueue
{
	be_t<u32> m_v1;
	be_t<u32> m_v2;
	be_t<u32> m_size;
	be_t<u32> m_depth;
	be_t<u64> m_addr;
	be_t<u64> reserved;

	volatile u64& m_data()
	{
		return *reinterpret_cast<u64*>(this);
	};
};

static_assert(sizeof(CellSyncQueue) == 32, "CellSyncQueue: wrong size");