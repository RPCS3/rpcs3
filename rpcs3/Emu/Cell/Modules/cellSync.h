#pragma once

#include "Utilities/BitField.h"

namespace vm { using namespace ps3; }

// Return Codes
enum CellSyncError : s32
{
	CELL_SYNC_ERROR_AGAIN                  = ERROR_CODE(0x80410101),
	CELL_SYNC_ERROR_INVAL                  = ERROR_CODE(0x80410102),
	CELL_SYNC_ERROR_NOSYS                  = ERROR_CODE(0x80410103),
	CELL_SYNC_ERROR_NOMEM                  = ERROR_CODE(0x80410104),
	CELL_SYNC_ERROR_SRCH                   = ERROR_CODE(0x80410105),
	CELL_SYNC_ERROR_NOENT                  = ERROR_CODE(0x80410106),
	CELL_SYNC_ERROR_NOEXEC                 = ERROR_CODE(0x80410107),
	CELL_SYNC_ERROR_DEADLK                 = ERROR_CODE(0x80410108),
	CELL_SYNC_ERROR_PERM                   = ERROR_CODE(0x80410109),
	CELL_SYNC_ERROR_BUSY                   = ERROR_CODE(0x8041010A),
	CELL_SYNC_ERROR_ABORT                  = ERROR_CODE(0x8041010C),
	CELL_SYNC_ERROR_FAULT                  = ERROR_CODE(0x8041010D),
	CELL_SYNC_ERROR_CHILD                  = ERROR_CODE(0x8041010E),
	CELL_SYNC_ERROR_STAT                   = ERROR_CODE(0x8041010F),
	CELL_SYNC_ERROR_ALIGN                  = ERROR_CODE(0x80410110),
	CELL_SYNC_ERROR_NULL_POINTER           = ERROR_CODE(0x80410111),
	CELL_SYNC_ERROR_NOT_SUPPORTED_THREAD   = ERROR_CODE(0x80410112),
	CELL_SYNC_ERROR_NO_NOTIFIER            = ERROR_CODE(0x80410113),
	CELL_SYNC_ERROR_NO_SPU_CONTEXT_STORAGE = ERROR_CODE(0x80410114),
};

enum CellSyncError1 : s32
{
	CELL_SYNC_ERROR_SHOTAGE                = ERROR_CODE(0x80410112),
	CELL_SYNC_ERROR_UNKNOWNKEY             = ERROR_CODE(0x80410113),
};

template<>
inline const char* ppu_error_code::print(CellSyncError error)
{
	switch (error)
	{
		STR_CASE(CELL_SYNC_ERROR_AGAIN);
		STR_CASE(CELL_SYNC_ERROR_INVAL);
		STR_CASE(CELL_SYNC_ERROR_NOSYS);
		STR_CASE(CELL_SYNC_ERROR_NOMEM);
		STR_CASE(CELL_SYNC_ERROR_SRCH);
		STR_CASE(CELL_SYNC_ERROR_NOENT);
		STR_CASE(CELL_SYNC_ERROR_NOEXEC);
		STR_CASE(CELL_SYNC_ERROR_DEADLK);
		STR_CASE(CELL_SYNC_ERROR_PERM);
		STR_CASE(CELL_SYNC_ERROR_BUSY);
		STR_CASE(CELL_SYNC_ERROR_ABORT);
		STR_CASE(CELL_SYNC_ERROR_FAULT);
		STR_CASE(CELL_SYNC_ERROR_CHILD);
		STR_CASE(CELL_SYNC_ERROR_STAT);
		STR_CASE(CELL_SYNC_ERROR_ALIGN);
		STR_CASE(CELL_SYNC_ERROR_NULL_POINTER);
		STR_CASE(CELL_SYNC_ERROR_NOT_SUPPORTED_THREAD);
		STR_CASE(CELL_SYNC_ERROR_NO_NOTIFIER);
		STR_CASE(CELL_SYNC_ERROR_NO_SPU_CONTEXT_STORAGE);
	}

	return nullptr;
}

template<>
inline const char* ppu_error_code::print(CellSyncError1 error)
{
	switch (error)
	{
		STR_CASE(CELL_SYNC_ERROR_SHOTAGE);
		STR_CASE(CELL_SYNC_ERROR_UNKNOWNKEY);
	}

	return nullptr;
}

namespace _sync
{
	struct alignas(4) mutex // CellSyncMutex control variable
	{
		be_t<u16> rel;
		be_t<u16> acq;
	};
}

struct CellSyncMutex
{
	atomic_t<_sync::mutex> ctrl;
};

CHECK_SIZE_ALIGN(CellSyncMutex, 4, 4);

namespace _sync
{
	struct alignas(4) barrier // CellSyncBarrier control variable
	{
		be_t<s16> value;
		be_t<u16> count;

		static inline bool try_notify(barrier& ctrl)
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
		};

		static inline bool try_wait(barrier& ctrl)
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
}

struct CellSyncBarrier
{
	atomic_t<_sync::barrier> ctrl;
};

CHECK_SIZE_ALIGN(CellSyncBarrier, 4, 4);

namespace _sync
{
	struct alignas(4) rwlock // CellSyncRwm control variable
	{
		be_t<u16> readers;
		be_t<u16> writers;

		static inline bool try_read_begin(rwlock& ctrl)
		{
			if (ctrl.writers)
			{
				return false;
			}

			ctrl.readers++;
			return true;
		}

		static inline bool try_read_end(rwlock& ctrl)
		{
			if (ctrl.readers == 0)
			{
				return false;
			}

			ctrl.readers--;
			return true;
		}

		static inline bool try_write_begin(rwlock& ctrl)
		{
			if (ctrl.writers)
			{
				return false;
			}

			ctrl.writers = 1;
			return true;
		}
	};
}

struct alignas(16) CellSyncRwm
{
	atomic_t<_sync::rwlock> ctrl;

	be_t<u32> size;
	vm::bptr<void, u64> buffer;
};

CHECK_SIZE_ALIGN(CellSyncRwm, 16, 16);

namespace _sync
{
	struct alignas(8) queue // CellSyncQueue control variable
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

		static inline bool try_push_begin(queue& ctrl, u32 depth, u32* position)
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

		static inline bool try_pop_begin(queue& ctrl, u32 depth, u32* position)
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

		static inline bool try_peek_begin(queue& ctrl, u32 depth, u32* position)
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

		static inline bool try_clear_begin_1(queue& ctrl)
		{
			if (ctrl._pop)
			{
				return false;
			}

			ctrl._pop = 1;
			return true;
		}

		static inline bool try_clear_begin_2(queue& ctrl)
		{
			if (ctrl._push)
			{
				return false;
			}

			ctrl._push = 1;
			return true;
		}
	};
}

struct alignas(32) CellSyncQueue
{
	atomic_t<_sync::queue> ctrl;

	be_t<u32> size;
	be_t<u32> depth;
	vm::bptr<u8, u64> buffer;
	be_t<u64> reserved;

	u32 check_depth() const
	{
		const auto data = ctrl.load();

		if (data.next > depth || data.count > depth)
		{
			throw EXCEPTION("Invalid queue pointers");
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

struct alignas(128) CellSyncLFQueue
{
	struct alignas(8) pop1_t
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

	struct alignas(4) pop3_t
	{
		be_t<u16> m_h1;
		be_t<u16> m_h2;
	};

	struct alignas(8) push1_t
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

	struct alignas(4) push3_t
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
ppu_error_code cellSyncLFQueueInitialize(vm::ptr<CellSyncLFQueue> queue, vm::cptr<void> buffer, u32 size, u32 depth, u32 direction, vm::ptr<void> eaSignal);
