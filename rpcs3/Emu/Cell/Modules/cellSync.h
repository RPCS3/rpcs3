#pragma once

#include "Emu/Memory/vm_ptr.h"

#include "Utilities/BitField.h"

#include "Emu/Cell/ErrorCodes.h"

// Return Codes
enum CellSyncError : u32
{
	CELL_SYNC_ERROR_AGAIN                  = 0x80410101,
	CELL_SYNC_ERROR_INVAL                  = 0x80410102,
	CELL_SYNC_ERROR_NOSYS                  = 0x80410103,
	CELL_SYNC_ERROR_NOMEM                  = 0x80410104,
	CELL_SYNC_ERROR_SRCH                   = 0x80410105,
	CELL_SYNC_ERROR_NOENT                  = 0x80410106,
	CELL_SYNC_ERROR_NOEXEC                 = 0x80410107,
	CELL_SYNC_ERROR_DEADLK                 = 0x80410108,
	CELL_SYNC_ERROR_PERM                   = 0x80410109,
	CELL_SYNC_ERROR_BUSY                   = 0x8041010A,
	CELL_SYNC_ERROR_ABORT                  = 0x8041010C,
	CELL_SYNC_ERROR_FAULT                  = 0x8041010D,
	CELL_SYNC_ERROR_CHILD                  = 0x8041010E,
	CELL_SYNC_ERROR_STAT                   = 0x8041010F,
	CELL_SYNC_ERROR_ALIGN                  = 0x80410110,
	CELL_SYNC_ERROR_NULL_POINTER           = 0x80410111,
	CELL_SYNC_ERROR_NOT_SUPPORTED_THREAD   = 0x80410112,
	CELL_SYNC_ERROR_NO_NOTIFIER            = 0x80410113,
	CELL_SYNC_ERROR_NO_SPU_CONTEXT_STORAGE = 0x80410114,
};

enum CellSyncError1 : u32
{
	CELL_SYNC_ERROR_SHOTAGE                = 0x80410112,
	CELL_SYNC_ERROR_UNKNOWNKEY             = 0x80410113,
};

struct CellSyncMutex
{
	struct Counter
	{
		be_t<u16> rel;
		be_t<u16> acq;

		auto lock_begin()
		{
			return acq++;
		}

		bool try_lock()
		{
			if (rel != acq) [[unlikely]]
			{
				return false;
			}

			acq++;
			return true;
		}

		void unlock()
		{
			rel++;
		}
	};

	atomic_t<Counter> ctrl;
};

CHECK_SIZE_ALIGN(CellSyncMutex, 4, 4);

struct CellSyncBarrier
{
	struct alignas(4) ctrl_t
	{
		be_t<s16> value;
		be_t<u16> count;
	};

	atomic_t<ctrl_t> ctrl;

	static inline bool try_notify(ctrl_t& ctrl)
	{
		if (ctrl.value & 0x8000)
		{
			return false;
		}

		if (++ctrl.value == ctrl.count)
		{
			ctrl.value |= 0x8000;
		}

		return true;
	}

	static inline bool try_wait(ctrl_t& ctrl)
	{
		if ((ctrl.value & 0x8000) == 0)
		{
			return false;
		}

		if (--ctrl.value == -0x8000)
		{
			ctrl.value = 0;
		}

		return true;
	}
};

CHECK_SIZE_ALIGN(CellSyncBarrier, 4, 4);

struct alignas(16) CellSyncRwm
{
	struct alignas(4) ctrl_t
	{
		be_t<u16> readers;
		be_t<u16> writers;
	};

	atomic_t<ctrl_t> ctrl;

	be_t<u32> size;
	vm::bptr<void, u64> buffer;

	static inline bool try_read_begin(ctrl_t& ctrl)
	{
		if (ctrl.writers)
		{
			return false;
		}

		ctrl.readers++;
		return true;
	}

	static inline bool try_read_end(ctrl_t& ctrl)
	{
		if (ctrl.readers == 0)
		{
			return false;
		}

		ctrl.readers--;
		return true;
	}

	static inline bool try_write_begin(ctrl_t& ctrl)
	{
		if (ctrl.writers)
		{
			return false;
		}

		ctrl.writers = 1;
		return true;
	}
};

CHECK_SIZE_ALIGN(CellSyncRwm, 16, 16);

struct alignas(32) CellSyncQueue
{
	struct ctrl_t
	{
		union
		{
			be_t<u32> x0;

			bf_t<be_t<u32>, 0, 24> next;
			bf_t<be_t<u32>, 24, 8> _pop;
		};

		union
		{
			be_t<u32> x4;

			bf_t<be_t<u32>, 0, 24> count;
			bf_t<be_t<u32>, 24, 8> _push;
		};
	};

	atomic_t<ctrl_t> ctrl;

	be_t<u32> size;
	be_t<u32> depth;
	vm::bptr<u8, u64> buffer;
	be_t<u64> reserved;

	u32 check_depth() const
	{
		const auto data = ctrl.load();

		if (data.next > depth || data.count > depth)
		{
			fmt::throw_exception("Invalid queue pointers");
		}

		return depth;
	}

	static inline bool try_push_begin(ctrl_t& ctrl, u32 depth, u32* position)
	{
		const u32 count = ctrl.count;

		if (ctrl._push || count + ctrl._pop >= depth)
		{
			return false;
		}

		*position = ctrl.next;
		ctrl.next = *position + 1 != depth ? *position + 1 : 0;
		ctrl.count = count + 1;
		ctrl._push = 1;
		return true;
	}

	static inline void push_end(ctrl_t& ctrl)
	{
		ctrl._push = 0;
	}

	static inline bool try_pop_begin(ctrl_t& ctrl, u32 depth, u32* position)
	{
		const u32 count = ctrl.count;

		if (ctrl._pop || count <= ctrl._push)
		{
			return false;
		}

		ctrl._pop = 1;
		*position = ctrl.next + depth - count;
		ctrl.count = count - 1;
		return true;
	}

	static inline bool try_peek_begin(ctrl_t& ctrl, u32 depth, u32* position)
	{
		const u32 count = ctrl.count;

		if (ctrl._pop || count <= ctrl._push)
		{
			return false;
		}

		ctrl._pop = 1;
		*position = ctrl.next + depth - count;
		return true;
	}

	static inline void pop_end(ctrl_t& ctrl)
	{
		ctrl._pop = 0;
	}

	static inline bool try_clear_begin_1(ctrl_t& ctrl)
	{
		if (ctrl._pop)
		{
			return false;
		}

		ctrl._pop = 1;
		return true;
	}

	static inline bool try_clear_begin_2(ctrl_t& ctrl)
	{
		if (ctrl._push)
		{
			return false;
		}

		ctrl._push = 1;
		return true;
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

struct alignas(128) CellSyncLFQueue
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
		atomic_t<pop1_t> pop1;
		atomic_t<pop3_t> pop3;
	};

	union // 0x8
	{
		atomic_t<push1_t> push1;
		atomic_t<push3_t> push3;
	};

	be_t<u32> m_size;              // 0x10
	be_t<u32> m_depth;             // 0x14
	vm::bcptr<void, u64> m_buffer; // 0x18
	u8 m_bs[4];                    // 0x20
	be_t<u32> m_direction;         // 0x24 CellSyncQueueDirection
	be_t<u32> m_v1;                // 0x28
	atomic_be_t<s32> init;         // 0x2C
	atomic_t<push2_t> push2;       // 0x30
	be_t<u16> m_hs1[15];           // 0x32
	atomic_t<pop2_t> pop2;         // 0x50
	be_t<u16> m_hs2[15];           // 0x52
	vm::bptr<void, u64> m_eaSignal; // 0x70
	be_t<u32> m_v2;                // 0x78
	be_t<u32> m_eq_id;             // 0x7C
};

CHECK_SIZE_ALIGN(CellSyncLFQueue, 128, 128);

// Prototypes
error_code cellSyncLFQueueInitialize(vm::ptr<CellSyncLFQueue> queue, vm::cptr<void> buffer, u32 size, u32 depth, u32 direction, vm::ptr<void> eaSignal);
