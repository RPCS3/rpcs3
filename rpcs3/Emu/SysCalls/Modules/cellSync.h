#pragma once

namespace vm { using namespace ps3; }

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

struct set_alignment(4) sync_mutex_t // CellSyncMutex sync var
{
	be_t<u16> rel;
	be_t<u16> acq;

	be_t<u16> acquire()
	{
		return acq++;
	}

	bool try_lock()
	{
		return acq++ == rel;
	}

	void unlock()
	{
		rel++;
	}
};

using CellSyncMutex = atomic_be_t<sync_mutex_t>;

CHECK_SIZE_ALIGN(CellSyncMutex, 4, 4);

struct set_alignment(4) sync_barrier_t // CellSyncBarrier sync var
{
	be_t<s16> value;
	be_t<s16> count;

	bool try_notify()
	{
		// extract m_value (repeat if < 0), increase, compare with second s16, set sign bit if equal, insert it back
		s16 v = value;

		if (v < 0)
		{
			return false;
		}

		if (++v == count)
		{
			v |= 0x8000;
		}

		value = v;

		return true;
	};

	bool try_wait()
	{
		// extract m_value (repeat if >= 0), decrease it, set 0 if == 0x8000, insert it back
		s16 v = value;

		if (v >= 0)
		{
			return false;
		}

		if (--v == -0x8000)
		{
			v = 0;
		}

		value = v;

		return true;
	}
};

using CellSyncBarrier = atomic_be_t<sync_barrier_t>;

CHECK_SIZE_ALIGN(CellSyncBarrier, 4, 4);

struct sync_rwm_t // CellSyncRwm sync var
{
	be_t<u16> readers;
	be_t<u16> writers;

	bool try_read_begin()
	{
		if (writers.data())
		{
			return false;
		}

		readers++;
		return true;
	}

	bool try_read_end()
	{
		if (!readers.data())
		{
			return false;
		}

		readers--;
		return true;
	}

	bool try_write_begin()
	{
		if (writers.data())
		{
			return false;
		}

		writers = 1;
		return true;
	}
};

struct set_alignment(16) CellSyncRwm
{
	atomic_be_t<sync_rwm_t> ctrl; // sync var

	be_t<u32> size;
	vm::bptr<void, u64> buffer;
};

CHECK_SIZE_ALIGN(CellSyncRwm, 16, 16);

struct sync_queue_t // CellSyncQueue sync var
{
	be_t<u32> m_v1;
	be_t<u32> m_v2;

	bool try_push(u32 depth, u32& position)
	{
		const u32 v1 = m_v1;
		const u32 v2 = m_v2;

		// compare 5th byte with zero (break if not zero)
		// compare (second u32 (u24) + first byte) with depth (break if greater or equal)
		if ((v2 >> 24) || ((v2 & 0xffffff) + (v1 >> 24)) >= depth)
		{
			return false;
		}

		// extract first u32 (u24) (-> position), calculate (position + 1) % depth, insert it back
		// insert 1 in 5th u8
		// extract second u32 (u24), increase it, insert it back
		position = (v1 & 0xffffff);
		m_v1 = (v1 & 0xff000000) | ((position + 1) % depth);
		m_v2 = (1 << 24) | ((v2 & 0xffffff) + 1);

		return true;
	}

	bool try_pop(u32 depth, u32& position)
	{
		const u32 v1 = m_v1;
		const u32 v2 = m_v2;

		// extract first u8, repeat if not zero
		// extract second u32 (u24), subtract 5th u8, compare with zero, repeat if less or equal
		if ((v1 >> 24) || ((v2 & 0xffffff) <= (v2 >> 24)))
		{
			return false;
		}

		// insert 1 in first u8
		// extract first u32 (u24), add depth, subtract second u32 (u24), calculate (% depth), save to position
		// extract second u32 (u24), decrease it, insert it back
		m_v1 = 0x1000000 | v1;
		position = ((v1 & 0xffffff) + depth - (v2 & 0xffffff)) % depth;
		m_v2 = (v2 & 0xff000000) | ((v2 & 0xffffff) - 1);

		return true;
	}

	bool try_peek(u32 depth, u32& position)
	{
		const u32 v1 = m_v1;
		const u32 v2 = m_v2;

		if ((v1 >> 24) || ((v2 & 0xffffff) <= (v2 >> 24)))
		{
			return false;
		}

		m_v1 = 0x1000000 | v1;
		position = ((v1 & 0xffffff) + depth - (v2 & 0xffffff)) % depth;

		return true;
	}
};

struct set_alignment(32) CellSyncQueue
{
	atomic_be_t<sync_queue_t> ctrl;

	be_t<u32> size;
	be_t<u32> depth;
	vm::bptr<u8, u64> buffer;
	be_t<u64> reserved;

	u32 check_depth()
	{
		const auto data = ctrl.load();

		if ((data.m_v1 & 0xffffff) > depth || (data.m_v2 & 0xffffff) > depth)
		{
			throw __FUNCTION__;
		}

		return depth;
	}
};

CHECK_SIZE_ALIGN(CellSyncQueue, 32, 32);

enum CellSyncQueueDirection : u32 // CellSyncLFQueueDirection
{
	CELL_SYNC_QUEUE_SPU2SPU = 0, // SPU to SPU
	CELL_SYNC_QUEUE_SPU2PPU = 1, // SPU to PPU
	CELL_SYNC_QUEUE_PPU2SPU = 2, // PPU to SPU
	CELL_SYNC_QUEUE_ANY2ANY = 3, // SPU/PPU to SPU/PPU
};

struct set_alignment(128) CellSyncLFQueue
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
	vm::bptr<u8, u64> m_buffer;    // 0x18
	u8 m_bs[4];                    // 0x20
	be_t<u32> m_direction;         // 0x24 CellSyncQueueDirection
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

CHECK_SIZE_ALIGN(CellSyncLFQueue, 128, 128);

s32 syncLFQueueInitialize(vm::ptr<CellSyncLFQueue> queue, vm::ptr<u8> buffer, u32 size, u32 depth, CellSyncQueueDirection direction, vm::ptr<void> eaSignal);
s32 syncLFQueueGetPushPointer(PPUThread& CPU, vm::ptr<CellSyncLFQueue> queue, s32& pointer, u32 isBlocking, u32 useEventQueue);
s32 syncLFQueueGetPushPointer2(PPUThread& CPU, vm::ptr<CellSyncLFQueue> queue, s32& pointer, u32 isBlocking, u32 useEventQueue);
s32 syncLFQueueCompletePushPointer(PPUThread& CPU, vm::ptr<CellSyncLFQueue> queue, s32 pointer, std::function<s32(PPUThread& CPU, u32 addr, u32 arg)> fpSendSignal);
s32 syncLFQueueCompletePushPointer2(PPUThread& CPU, vm::ptr<CellSyncLFQueue> queue, s32 pointer, std::function<s32(PPUThread& CPU, u32 addr, u32 arg)> fpSendSignal);
s32 syncLFQueueGetPopPointer(PPUThread& CPU, vm::ptr<CellSyncLFQueue> queue, s32& pointer, u32 isBlocking, u32, u32 useEventQueue);
s32 syncLFQueueGetPopPointer2(PPUThread& CPU, vm::ptr<CellSyncLFQueue> queue, s32& pointer, u32 isBlocking, u32 useEventQueue);
s32 syncLFQueueCompletePopPointer(PPUThread& CPU, vm::ptr<CellSyncLFQueue> queue, s32 pointer, std::function<s32(PPUThread& CPU, u32 addr, u32 arg)> fpSendSignal, u32 noQueueFull);
s32 syncLFQueueCompletePopPointer2(PPUThread& CPU, vm::ptr<CellSyncLFQueue> queue, s32 pointer, std::function<s32(PPUThread& CPU, u32 addr, u32 arg)> fpSendSignal, u32 noQueueFull);
s32 syncLFQueueAttachLv2EventQueue(vm::ptr<u32> spus, u32 num, vm::ptr<CellSyncLFQueue> queue);
s32 syncLFQueueDetachLv2EventQueue(vm::ptr<u32> spus, u32 num, vm::ptr<CellSyncLFQueue> queue);
