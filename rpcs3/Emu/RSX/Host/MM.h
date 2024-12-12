#pragma once

#include <util/types.hpp>
#include <util/vm.hpp>

namespace rsx
{
	struct MM_block
	{
		u64 start;
		u64 length;
		utils::protection prot;

		inline bool overlaps(u64 start, u64 end) const
		{
			// [Start, End] is not a proper closed range, there is an off-by-one by design.
			// FIXME: Use address_range64
			const u64 this_end = this->start + this->length;
			return (this->start < end && start < this_end);
		}

		inline bool overlaps(u64 addr) const
		{
			// [Start, End] is not a proper closed range, there is an off-by-one by design.
			// FIXME: Use address_range64
			const u64 this_end = this->start + this->length;
			return (addr >= start && addr < this_end);
		}
	};

	enum mm_backend_ctrl : u32
	{
		cmd_mm_flush = 0x81000000,
	};

	void mm_protect(void* start, u64 length, utils::protection prot);
	void mm_flush_lazy();
	void mm_flush(u32 vm_address);
	void mm_flush();
}
