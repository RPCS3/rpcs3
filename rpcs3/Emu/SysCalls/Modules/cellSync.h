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

union CellSyncMutex
{
	struct sync_t
	{
		be_t<u16> release_count; // increased when mutex is unlocked
		be_t<u16> acquire_count; // increased when mutex is locked
	};

	struct
	{
		atomic_be_t<u16> release_count;
		atomic_be_t<u16> acquire_count;
	};

	atomic_be_t<sync_t> sync_var;
};

static_assert(sizeof(CellSyncMutex) == 4, "CellSyncMutex: wrong size");

struct CellSyncBarrier
{
	struct data_t
	{
		be_t<s16> m_value;
		be_t<s16> m_count;
	};

	atomic_be_t<data_t> data;
};

static_assert(sizeof(CellSyncBarrier) == 4, "CellSyncBarrier: wrong size");

struct CellSyncRwm
{
	struct data_t
	{
		be_t<u16> m_readers;
		be_t<u16> m_writers;
	};

	atomic_be_t<data_t> data;
	be_t<u32> m_size;
	vm::bptr<void, u64> m_buffer;
};

static_assert(sizeof(CellSyncRwm) == 16, "CellSyncRwm: wrong size");

struct CellSyncQueue
{
	struct data_t
	{
		be_t<u32> m_v1;
		be_t<u32> m_v2;
	};

	atomic_be_t<data_t> data;
	be_t<u32> m_size;
	be_t<u32> m_depth;
	vm::bptr<u8, u64> m_buffer;
	be_t<u64> reserved;
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
	struct pop1_t
	{
		be_t<u16> m_h1;
		be_t<u16> m_h2;
		be_t<u16> m_h3;
		be_t<u16> m_h4;
	};

	struct pop2_t
	{
		be_t<u16> pack;
	};

	struct pop3_t
	{
		be_t<u16> m_h1;
		be_t<u16> m_h2;
	};

	struct push1_t
	{
		be_t<u16> m_h5;
		be_t<u16> m_h6;
		be_t<u16> m_h7;
		be_t<u16> m_h8;
	};

	struct push2_t
	{
		be_t<u16> pack;
	};

	struct push3_t
	{
		be_t<u16> m_h5;
		be_t<u16> m_h6;
	};

	union // 0x0
	{
		atomic_be_t<pop1_t> pop1;
		atomic_be_t<pop3_t> pop3;
	};

	union // 0x8
	{
		atomic_be_t<push1_t> push1;
		atomic_be_t<push3_t> push3;
	};

	be_t<u32> m_size;              // 0x10
	be_t<u32> m_depth;             // 0x14
	vm::bptr<u8, u64> m_buffer; // 0x18
	u8 m_bs[4];                    // 0x20
	be_t<CellSyncQueueDirection> m_direction; // 0x24
	be_t<u32> m_v1;                // 0x28
	atomic_be_t<u32> init;         // 0x2C
	atomic_be_t<push2_t> push2;    // 0x30
	be_t<u16> m_hs1[15];           // 0x32
	atomic_be_t<pop2_t> pop2;      // 0x50
	be_t<u16> m_hs2[15];           // 0x52
	vm::bptr<void, u64> m_eaSignal; // 0x70
	be_t<u32> m_v2;                // 0x78
	be_t<u32> m_eq_id;             // 0x7C

	std::string dump()
	{
		std::string res = "CellSyncLFQueue dump:";

		auto data = (be_t<u64>*)this;

		for (u32 i = 0; i < sizeof(CellSyncLFQueue) / sizeof(u64); i += 2)
		{
			res += "\n*** 0x";
			res += fmt::to_hex(data[i + 0], 16);
			res += " 0x";
			res += fmt::to_hex(data[i + 1], 16);
		}

		return res;
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
s32 syncLFQueueAttachLv2EventQueue(vm::ptr<u32> spus, u32 num, vm::ptr<CellSyncLFQueue> queue);
s32 syncLFQueueDetachLv2EventQueue(vm::ptr<u32> spus, u32 num, vm::ptr<CellSyncLFQueue> queue);
