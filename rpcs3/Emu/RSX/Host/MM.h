#pragma once

#include <util/types.hpp>
#include <util/vm.hpp>

#include "Emu/RSX/Common/simple_array.hpp"
#include "Utilities/address_range.h"

namespace rsx
{
	struct MM_block
	{
		utils::address_range64 range;
		utils::protection prot;
		u64 sync_tag;

		inline void merge(const utils::address_range64& other)
		{
			AUDIT(other.valid());
			range = utils::address_range64::start_end(
				std::min(range.start, other.start),
				std::max(range.end, other.end)
			);
		}

		inline bool overlaps(const utils::address_range64& test) const
		{
			return range.overlaps(test);
		}

		inline bool overlaps(u64 addr) const
		{
			return range.overlaps(addr);
		}

		inline bool touches(const utils::address_range64& test) const
		{
			return range.touches(test);
		}
	};

	enum mm_backend_ctrl : u32
	{
		cmd_mm_flush = 0x81000000,
	};

	void mm_protect(void* start, u64 length, utils::protection prot);
	void mm_flush_lazy();
	void mm_flush(u32 vm_address);
	void mm_flush(const rsx::simple_array<utils::address_range64>& ranges);
	void mm_flush_partial(u64 tag);
	void mm_flush();
}
