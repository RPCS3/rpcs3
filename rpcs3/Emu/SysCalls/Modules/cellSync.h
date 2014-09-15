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
	vm::bptr<void, 1, u64> m_buffer;

	volatile u32& m_data()
	{
		return *reinterpret_cast<u32*>(this);
	};
};

static_assert(sizeof(CellSyncRwm) == 16, "CellSyncRwm: wrong size");

struct CellSyncQueue
{
	be_t<u32> m_v1;
	be_t<u32> m_v2;
	be_t<u32> m_size;
	be_t<u32> m_depth;
	vm::bptr<u8, 1, u64> m_buffer;
	be_t<u64> reserved;

	volatile u64& m_data()
	{
		return *reinterpret_cast<u64*>(this);
	};
};

static_assert(sizeof(CellSyncQueue) == 32, "CellSyncQueue: wrong size");

enum CellSyncQueueDirection : u32 // CellSyncLFQueueDirection
{
	CELL_SYNC_QUEUE_SPU2SPU = 0, // SPU to SPU
	CELL_SYNC_QUEUE_SPU2PPU = 1, // SPU to PPU
	CELL_SYNC_QUEUE_PPU2SPU = 2, // PPU to SPU
	CELL_SYNC_QUEUE_ANY2ANY = 3, // SPU/PPU to SPU/PPU
};

struct CellSyncLFQueue
{
	be_t<u16> m_h1;      // 0x0
	be_t<u16> m_h2;      // 0x2
	be_t<u16> m_h3;      // 0x4
	be_t<u16> m_h4;      // 0x6
	be_t<u16> m_h5;      // 0x8
	be_t<u16> m_h6;      // 0xA
	be_t<u16> m_h7;      // 0xC
	be_t<u16> m_h8;      // 0xE
	be_t<u32> m_size;    // 0x10
	be_t<u32> m_depth;   // 0x14
	vm::bptr<u8, 1, u64> m_buffer; // 0x18
	u8        m_bs[4];   // 0x20
	be_t<CellSyncQueueDirection> m_direction; // 0x24
	be_t<u32> m_v1;      // 0x28
	be_t<u32> m_sync;    // 0x2C
	be_t<u16> m_hs[32];  // 0x30
	vm::bptr<void, 1, u64> m_eaSignal; // 0x70
	be_t<u32> m_v2;      // 0x78
	be_t<u32> m_v3;      // 0x7C

	volatile u32& m_data()
	{
		return *reinterpret_cast<u32*>((u8*)this + 0x2c);
	}

	volatile u64& m_push1()
	{
		return *reinterpret_cast<u64*>((u8*)this + 0x8);
	}

	volatile u32& m_push2()
	{
		return *reinterpret_cast<u32*>((u8*)this + 0x30);
	}

	volatile u32& m_push3()
	{
		return *reinterpret_cast<u32*>((u8*)this + 0x8);
	}

	volatile u64& m_pop1()
	{
		return *reinterpret_cast<u64*>((u8*)this + 0x0);
	}

	volatile u32& m_pop2()
	{
		return *reinterpret_cast<u32*>((u8*)this + 0x50);
	}

	volatile u32& m_pop3()
	{
		return *reinterpret_cast<u32*>((u8*)this + 0x0);
	}
};

static_assert(sizeof(CellSyncLFQueue) == 128, "CellSyncLFQueue: wrong size");

s32 syncMutexInitialize(vm::ptr<CellSyncMutex> mutex);

s32 syncBarrierInitialize(vm::ptr<CellSyncBarrier> barrier, u16 total_count);

s32 syncRwmInitialize(vm::ptr<CellSyncRwm> rwm, vm::ptr<void> buffer, u32 buffer_size);

s32 syncQueueInitialize(vm::ptr<CellSyncQueue> queue, vm::ptr<u8> buffer, u32 size, u32 depth);

s32 syncLFQueueInitialize(vm::ptr<CellSyncLFQueue> queue, vm::ptr<u8> buffer, u32 size, u32 depth, CellSyncQueueDirection direction, vm::ptr<void> eaSignal);
s32 syncLFQueueGetPushPointer(vm::ptr<CellSyncLFQueue> queue, s32& pointer, u32 isBlocking, u32 useEventQueue);
s32 syncLFQueueGetPushPointer2(vm::ptr<CellSyncLFQueue> queue, s32& pointer, u32 isBlocking, u32 useEventQueue);
s32 syncLFQueueCompletePushPointer(vm::ptr<CellSyncLFQueue> queue, s32 pointer, const std::function<s32(u32 addr, u32 arg)> fpSendSignal);
s32 syncLFQueueCompletePushPointer2(vm::ptr<CellSyncLFQueue> queue, s32 pointer, const std::function<s32(u32 addr, u32 arg)> fpSendSignal);
s32 syncLFQueueGetPopPointer(vm::ptr<CellSyncLFQueue> queue, s32& pointer, u32 isBlocking, u32, u32 useEventQueue);
s32 syncLFQueueGetPopPointer2(vm::ptr<CellSyncLFQueue> queue, s32& pointer, u32 isBlocking, u32 useEventQueue);
s32 syncLFQueueCompletePopPointer(vm::ptr<CellSyncLFQueue> queue, s32 pointer, const std::function<s32(u32 addr, u32 arg)> fpSendSignal, u32 noQueueFull);
s32 syncLFQueueCompletePopPointer2(vm::ptr<CellSyncLFQueue> queue, s32 pointer, const std::function<s32(u32 addr, u32 arg)> fpSendSignal, u32 noQueueFull);
s32 syncLFQueueAttachLv2EventQueue(vm::ptr<be_t<u32>> spus, u32 num, vm::ptr<CellSyncLFQueue> queue);
s32 syncLFQueueDetachLv2EventQueue(vm::ptr<be_t<u32>> spus, u32 num, vm::ptr<CellSyncLFQueue> queue);
