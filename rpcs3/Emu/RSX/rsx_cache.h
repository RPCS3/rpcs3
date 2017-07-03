#pragma once
#include "Utilities/VirtualMemory.h"
#include "Emu/Memory/vm.h"
#include "gcm_enums.h"

namespace rsx
{
	struct blit_src_info
	{
		blit_engine::transfer_source_format format;
		u16 offset_x;
		u16 offset_y;
		u16 width;
		u16 height;
		u16 slice_h;
		u16 pitch;
		void *pixels;

		u32 rsx_address;
	};

	struct blit_dst_info
	{
		blit_engine::transfer_destination_format format;
		u16 offset_x;
		u16 offset_y;
		u16 width;
		u16 height;
		u16 pitch;
		u16 clip_x;
		u16 clip_y;
		u16 clip_width;
		u16 clip_height;

		bool swizzled;
		void *pixels;

		u32  rsx_address;
	};

	enum protection_policy
	{
		protect_policy_one_page,	//Only guard one page, preferrably one where this section 'wholly' fits
		protect_policy_full_range	//Guard the full memory range. Shared pages may be invalidated by access outside the object we're guarding
	};

	class buffered_section
	{
	private:
		u32 locked_address_base = 0;
		u32 locked_address_range = 0;

	protected:
		u32 cpu_address_base = 0;
		u32 cpu_address_range = 0;

		utils::protection protection = utils::protection::rw;

		bool locked = false;
		bool dirty = false;

		inline bool region_overlaps(u32 base1, u32 limit1, u32 base2, u32 limit2)
		{
			return (base1 < limit2 && base2 < limit1);
		}

	public:

		buffered_section() {}
		~buffered_section() {}

		void reset(u32 base, u32 length, protection_policy protect_policy= protect_policy_full_range)
		{
			verify(HERE), locked == false;

			cpu_address_base = base;
			cpu_address_range = length;

			locked_address_base = (base & ~4095);

			if (protect_policy == protect_policy_one_page)
			{
				locked_address_range = 4096;
				if (locked_address_base < base)
				{
					//Try the next page if we can
					//TODO: If an object spans a boundary without filling either side, guard the larger page occupancy
					const u32 next_page = locked_address_base + 4096;
					if ((base + length) >= (next_page + 4096))
					{
						//The object spans the entire page. Guard this instead
						locked_address_base = next_page;
					}
				}
			}
			else
				locked_address_range = align(base + length, 4096) - locked_address_base;

			protection = utils::protection::rw;
			locked = false;
		}

		void protect(utils::protection prot)
		{
			if (prot == protection) return;

			utils::memory_protect(vm::base(locked_address_base), locked_address_range, prot);
			protection = prot;
			locked = prot != utils::protection::rw;
		}

		void unprotect()
		{
			protect(utils::protection::rw);
			locked = false;
		}

		bool overlaps(std::pair<u32, u32> range)
		{
			return region_overlaps(locked_address_base, locked_address_base + locked_address_range, range.first, range.first + range.second);
		}

		bool overlaps(u32 address)
		{
			return (locked_address_base <= address && (address - locked_address_base) < locked_address_range);
		}

		/**
		 * Check if range overlaps with this section.
		 * ignore_protection_range - if true, the test should not check against the aligned protection range, instead
		 * tests against actual range of contents in memory
		 */
		bool overlaps(std::pair<u32, u32> range, bool ignore_protection_range)
		{
			if (!ignore_protection_range)
				return region_overlaps(locked_address_base, locked_address_base + locked_address_range, range.first, range.first + range.second);
			else
				return region_overlaps(cpu_address_base, cpu_address_base + cpu_address_range, range.first, range.first + range.second);
		}

		/**
		 * Check if the page containing the address tramples this section. Also compares a former trampled page range to compare
		 * If true, returns the range <min, max> with updated invalid range 
		 */
		std::tuple<bool, std::pair<u32, u32>> overlaps_page(std::pair<u32, u32> old_range, u32 address)
		{
			const u32 page_base = address & ~4095;
			const u32 page_limit = address + 4096;

			const u32 compare_min = std::min(old_range.first, page_base);
			const u32 compare_max = std::max(old_range.second, page_limit);

			if (!region_overlaps(locked_address_base, locked_address_base + locked_address_range, compare_min, compare_max))
				return std::make_tuple(false, old_range);

			return std::make_tuple(true, get_min_max(std::make_pair(compare_min, compare_max)));
		}

		bool is_locked() const
		{
			return locked;
		}

		bool is_dirty() const
		{
			return dirty;
		}

		void set_dirty(bool state)
		{
			dirty = state;
		}

		u32 get_section_base() const
		{
			return cpu_address_base;
		}

		u32 get_section_size() const
		{
			return cpu_address_range;
		}

		bool matches(u32 cpu_address, u32 size) const
		{
			return (cpu_address_base == cpu_address && cpu_address_range == size);
		}

		std::pair<u32, u32> get_min_max(std::pair<u32, u32> current_min_max)
		{
			u32 min = std::min(current_min_max.first, locked_address_base);
			u32 max = std::max(current_min_max.second, locked_address_base + locked_address_range);

			return std::make_pair(min, max);
		}
	};
}
