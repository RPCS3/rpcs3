#include "stdafx.h"
#include "texture_cache_utils.h"
#include "Utilities/address_range.h"
#include "util/fnv_hash.hpp"

namespace rsx
{
	constexpr u32 min_lockable_data_size = 4096; // Increasing this value has worse results even on systems with pages > 4k

	void buffered_section::init_lockable_range(const address_range32& range)
	{
		locked_range = range.to_page_range();
		AUDIT((locked_range.start == page_start(range.start)) || (locked_range.start == next_page(range.start)));
		AUDIT(locked_range.end <= page_end(range.end));
		ensure(locked_range.is_page_range());
	}

	void buffered_section::reset(const address_range32& memory_range)
	{
		ensure(memory_range.valid() && locked == false);

		cpu_range = address_range32(memory_range);
		confirmed_range.invalidate();
		locked_range.invalidate();

		protection = utils::protection::rw;
		protection_strat = section_protection_strategy::lock;
		locked = false;

		init_lockable_range(cpu_range);

		if (memory_range.length() < min_lockable_data_size)
		{
			protection_strat = section_protection_strategy::hash;
			mem_hash = 0;
		}
	}

	void buffered_section::invalidate_range()
	{
		ensure(!locked);

		cpu_range.invalidate();
		confirmed_range.invalidate();
		locked_range.invalidate();
	}

	void buffered_section::protect(utils::protection new_prot, bool force)
	{
		if (new_prot == protection && !force) return;

		ensure(locked_range.is_page_range());
		AUDIT(!confirmed_range.valid() || confirmed_range.inside(cpu_range));

#ifdef TEXTURE_CACHE_DEBUG
		if (new_prot != protection || force)
		{
			if (locked && !force) // When force=true, it is the responsibility of the caller to remove this section from the checker refcounting
				tex_cache_checker.remove(locked_range, protection);
			if (new_prot != utils::protection::rw)
				tex_cache_checker.add(locked_range, new_prot);
		}
#endif // TEXTURE_CACHE_DEBUG

		if (new_prot == utils::protection::no)
		{
			// Override
			protection_strat = section_protection_strategy::lock;
		}

		if (protection_strat == section_protection_strategy::lock)
		{
			rsx::memory_protect(locked_range, new_prot);
		}
		else if (new_prot != utils::protection::rw)
		{
			mem_hash = fast_hash_internal();
		}

		protection = new_prot;
		locked = (protection != utils::protection::rw);

		if (!locked)
		{
			// Unprotect range also invalidates secured range
			confirmed_range.invalidate();
		}
	}

	void buffered_section::protect(utils::protection prot, const std::pair<u32, u32>& new_confirm)
	{
		// new_confirm.first is an offset after cpu_range.start
		// new_confirm.second is the length (after cpu_range.start + new_confirm.first)

#ifdef TEXTURE_CACHE_DEBUG
		// We need to remove the lockable range from page_info as we will be re-protecting with force==true
		if (locked)
			tex_cache_checker.remove(locked_range, protection);
#endif

		// Save previous state to compare for changes
		const auto prev_confirmed_range = confirmed_range;

		if (prot != utils::protection::rw)
		{
			if (confirmed_range.valid())
			{
				confirmed_range.start = std::min(confirmed_range.start, cpu_range.start + new_confirm.first);
				confirmed_range.end = std::max(confirmed_range.end, cpu_range.start + new_confirm.first + new_confirm.second - 1);
			}
			else
			{
				confirmed_range = address_range32::start_length(cpu_range.start + new_confirm.first, new_confirm.second);
				ensure(!locked || locked_range.inside(confirmed_range.to_page_range()));
			}

			ensure(confirmed_range.inside(cpu_range));
			init_lockable_range(confirmed_range);
		}

		protect(prot, confirmed_range != prev_confirmed_range);
	}

	void buffered_section::unprotect()
	{
		AUDIT(protection != utils::protection::rw);
		protect(utils::protection::rw);
	}

	void buffered_section::discard()
	{
#ifdef TEXTURE_CACHE_DEBUG
		if (locked)
			tex_cache_checker.remove(locked_range, protection);
#endif

		protection = utils::protection::rw;
		confirmed_range.invalidate();
		locked = false;
	}

	const address_range32& buffered_section::get_bounds(section_bounds bounds) const
	{
		switch (bounds)
		{
		case section_bounds::full_range:
			return cpu_range;
		case section_bounds::locked_range:
			return locked_range;
		case section_bounds::confirmed_range:
			return confirmed_range.valid() ? confirmed_range : cpu_range;
		default:
			fmt::throw_exception("Unreachable");
		}
	}

	u64 buffered_section::fast_hash_internal() const
	{
		const auto hash_range = confirmed_range.valid() ? confirmed_range : cpu_range;
		const auto hash_length = hash_range.length();
		const auto cycles = hash_length / 8;
		auto rem = hash_length % 8;

		auto src = get_ptr<const char>(hash_range.start);
		auto data64 = reinterpret_cast<const u64*>(src);

		usz hash = rpcs3::fnv_seed;
		for (unsigned i = 0; i < cycles; ++i)
		{
			hash = rpcs3::hash64(hash, data64[i]);
		}

		if (rem) [[unlikely]] // Data often aligned to some power of 2
		{
			src += hash_length - rem;

			if (rem > 4)
			{
				hash = rpcs3::hash64(hash, *reinterpret_cast<const u32*>(src));
				src += 4;
			}

			if (rem > 2)
			{
				hash = rpcs3::hash64(hash, *reinterpret_cast<const u16*>(src));
				src += 2;
			}

			while (rem--)
			{
				hash = rpcs3::hash64(hash, *reinterpret_cast<const u8*>(src));
				src++;
			}
		}

		return hash;
	}

	bool buffered_section::is_locked(bool actual_page_flags) const
	{
		if (!actual_page_flags || !locked)
		{
			return locked;
		}

		return (protection_strat == section_protection_strategy::lock);
	}

	bool buffered_section::sync() const
	{
		if (protection_strat == section_protection_strategy::lock || !locked)
		{
			return true;
		}

		return (fast_hash_internal() == mem_hash);
	}
}
