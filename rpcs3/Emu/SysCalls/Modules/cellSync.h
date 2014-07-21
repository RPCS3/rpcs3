#pragma once

// Return Codes
enum
{
	CELL_SYNC_ERROR_AGAIN = 0x80410101,
	CELL_SYNC_ERROR_INVAL = 0x80410102,
	CELL_SYNC_ERROR_NOMEM = 0x80410104,
	CELL_SYNC_ERROR_DEADLK = 0x80410108,
	CELL_SYNC_ERROR_PERM = 0x80410109,
	CELL_SYNC_ERROR_BUSY = 0x8041010A,
	CELL_SYNC_ERROR_STAT = 0x8041010F,
	CELL_SYNC_ERROR_ALIGN = 0x80410110,
	CELL_SYNC_ERROR_NULL_POINTER = 0x80410111,
	CELL_SYNC_ERROR_NOT_SUPPORTED_THREAD = 0x80410112,
	CELL_SYNC_ERROR_NO_NOTIFIER = 0x80410113,
	CELL_SYNC_ERROR_NO_SPU_CONTEXT_STORAGE = 0x80410114,
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
	be_t<u16> m_value;
	be_t<u16> m_count;

	volatile u32& m_data()
	{
		return *reinterpret_cast<u32*>(this);
	};
};

static_assert(sizeof(CellSyncBarrier) == 4, "CellSyncBarrier: wrong size");