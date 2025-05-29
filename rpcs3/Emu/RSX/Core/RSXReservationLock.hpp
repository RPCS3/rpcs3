#pragma once

#include <util/types.hpp>
#include "../RSXThread.h"

namespace rsx
{
	template<bool IsFullLock = false, uint Stride = 128>
	class reservation_lock
	{
		u32 addr = 0;
		u32 length = 0;

		inline void lock_range(u32 addr, u32 length)
		{
			if (!get_current_renderer()->iomap_table.lock<IsFullLock, Stride>(addr, length, get_current_cpu_thread()))
			{
				length = 0;
			}

			this->addr = addr;
			this->length = length;
		}

	public:
		reservation_lock(u32 addr, u32 length)
		{
			if (g_cfg.core.rsx_accurate_res_access &&
				addr < constants::local_mem_base)
			{
				lock_range(addr, length);
			}
		}

		reservation_lock(u32 addr, u32 length, bool setting)
		{
			if (setting)
			{
				lock_range(addr, length);
			}
		}

		// Multi-range lock. If ranges overlap, the combined range will be acquired.
		// If ranges do not overlap, the first range that is in main memory will be acquired.
		reservation_lock(u32 dst_addr, u32 dst_length, u32 src_addr, u32 src_length)
		{
			if (!g_cfg.core.rsx_accurate_res_access)
			{
				return;
			}

			const auto range1 = utils::address_range32::start_length(dst_addr, dst_length);
			const auto range2 = utils::address_range32::start_length(src_addr, src_length);
			utils::address_range32 target_range;

			if (!range1.overlaps(range2)) [[likely]]
			{
				target_range = (dst_addr < constants::local_mem_base) ? range1 : range2;
			}
			else
			{
				// Very unlikely
				target_range = range1.get_min_max(range2);
			}

			if (target_range.start < constants::local_mem_base)
			{
				lock_range(target_range.start, target_range.length());
			}
		}

		// Very special utility for batched transfers (SPU related)
		template <typename T = void>
		void update_if_enabled(u32 addr, u32 _length, const std::add_pointer_t<T>& lock_release = std::add_pointer_t<void>{})
		{
			if (!length)
			{
				unlock();
				return;
			}

			// This check is not perfect but it covers the important cases fast (this check is only an optimization - forcing true disables it)
			const bool should_update =
				(this->addr / rsx_iomap_table::c_lock_stride) != (addr / rsx_iomap_table::c_lock_stride) ||  // Lock-addr and test-addr have different locks, update
				(addr % rsx_iomap_table::c_lock_stride + _length) > rsx_iomap_table::c_lock_stride;          // Test range spills beyond our base section

			if (!should_update)
			{
				return;
			}

			if constexpr (!std::is_void_v<T>)
			{
				// See SPUThread.cpp
				lock_release->release(0);
			}

			unlock();
			lock_range(addr, _length);
		}

		void unlock(bool destructor = false)
		{
			if (!length)
			{
				return;
			}

			get_current_renderer()->iomap_table.unlock<IsFullLock, Stride>(addr, length);

			if (!destructor)
			{
				length = 0;
			}
		}

		~reservation_lock()
		{
			unlock(true);
		}
	};
}
