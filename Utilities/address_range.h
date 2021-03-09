#pragma once

#include "util/types.hpp"
#include "StrFmt.h"
#include <vector>


namespace utils
{
	class address_range_vector;


	/**
	 * Constexprs
	 */
	constexpr inline u32 page_start(u32 addr)
	{
		return addr & ~4095u;
	}

	constexpr inline u32 next_page(u32 addr)
	{
		return page_start(addr + 4096u);
	}

	constexpr inline u32 page_end(u32 addr)
	{
		return next_page(addr) - 1;
	}

	constexpr inline u32 is_page_aligned(u32 addr)
	{
		return page_start(addr) == addr;
	}


	/**
	 * Address Range utility class
	 */
	class address_range
	{
	public:
		u32 start = UINT32_MAX; // First address in range
		u32 end = 0; // Last address

	private:
		// Helper constexprs
		static constexpr inline bool range_overlaps(u32 start1, u32 end1, u32 start2, u32 end2)
		{
			return (start1 <= end2 && start2 <= end1);
		}

		static constexpr inline bool address_overlaps(u32 address, u32 start, u32 end)
		{
			return (start <= address && address <= end);
		}

		static constexpr inline bool range_inside_range(u32 start1, u32 end1, u32 start2, u32 end2)
		{
			return (start1 >= start2 && end1 <= end2);
		}

		constexpr address_range(u32 _start, u32 _end) : start(_start), end(_end) {}

	public:
		// Constructors
		constexpr address_range() = default;

		static constexpr address_range start_length(u32 _start, u32 _length)
		{
			if (!_length)
			{
				return {};
			}

			return {_start, _start + (_length - 1)};
		}

		static constexpr address_range start_end(u32 _start, u32 _end)
		{
			return {_start, _end};
		}

		// Length
		u32 length() const
		{
			AUDIT(valid());
			return end - start + 1;
		}

		void set_length(const u32 new_length)
		{
			end = start + new_length - 1;
			ensure(valid());
		}

		u32 next_address() const
		{
			return end + 1;
		}

		u32 prev_address() const
		{
			return start - 1;
		}

		// Overlapping checks
		bool overlaps(const address_range &other) const
		{
			AUDIT(valid() && other.valid());
			return range_overlaps(start, end, other.start, other.end);
		}

		bool overlaps(const u32 addr) const
		{
			AUDIT(valid());
			return address_overlaps(addr, start, end);
		}

		bool inside(const address_range &other) const
		{
			AUDIT(valid() && other.valid());
			return range_inside_range(start, end, other.start, other.end);
		}

		inline bool inside(const address_range_vector &vec) const;
		inline bool overlaps(const address_range_vector &vec) const;

		bool touches(const address_range &other) const
		{
			AUDIT(valid() && other.valid());
			// returns true if there is overlap, or if sections are side-by-side
			return overlaps(other) || other.start == next_address() || other.end == prev_address();
		}

		// Utilities
		s32 signed_distance(const address_range &other) const
		{
			if (touches(other))
			{
				return 0;
			}

			// other after this
			if (other.start > end)
			{
				return static_cast<s32>(other.start - end - 1);
			}

			// this after other
			AUDIT(start > other.end);
			return -static_cast<s32>(start - other.end - 1);
		}

		u32 distance(const address_range &other) const
		{
			if (touches(other))
			{
				return 0;
			}

			// other after this
			if (other.start > end)
			{
				return (other.start - end - 1);
			}

			// this after other
			AUDIT(start > other.end);
			return (start - other.end - 1);
		}

		address_range get_min_max(const address_range &other) const
		{
			return {
				std::min(valid() ? start : UINT32_MAX, other.valid() ? other.start : UINT32_MAX),
				std::max(valid() ? end : 0, other.valid() ? other.end : 0)
			};
		}

		void set_min_max(const address_range &other)
		{
			*this = get_min_max(other);
		}

		bool is_page_range() const
		{
			return (valid() && start % 4096u == 0 && length() % 4096u == 0);
		}

		address_range to_page_range() const
		{
			AUDIT(valid());
			return { page_start(start), page_end(end) };
		}

		void page_align()
		{
			AUDIT(valid());
			start = page_start(start);
			end = page_end(end);
			AUDIT(is_page_range());
		}

		address_range get_intersect(const address_range &clamp) const
		{
			if (!valid() || !clamp.valid())
			{
				return {};
			}

			return { std::max(start, clamp.start), std::min(end, clamp.end) };
		}

		void intersect(const address_range &clamp)
		{
			if (!clamp.valid())
			{
				invalidate();
			}
			else
			{
				start = std::max(start, clamp.start);
				end = std::min(end, clamp.end);
			}
		}

		// Validity
		bool valid() const
		{
			return (start <= end);
		}

		void invalidate()
		{
			start = UINT32_MAX;
			end = 0;
		}

		// Comparison Operators
		bool operator ==(const address_range &other) const
		{
			return (start == other.start && end == other.end);
		}

		bool operator !=(const address_range &other) const
		{
			return (start != other.start || end != other.end);
		}

		/**
		 * Debug
		 */
		std::string str() const
		{
			return fmt::format("{0x%x->0x%x}", start, end);
		}
	};

	static inline address_range page_for(u32 addr)
	{
		return address_range::start_end(page_start(addr), page_end(addr));
	}


	/**
	 * Address Range Vector utility class
	 *
	 * Collection of address_range objects. Allows for merging and removing ranges from the set.
	 */
	class address_range_vector
	{
	public:
		using vector_type = typename std::vector<address_range>;
		using iterator = typename vector_type::iterator;
		using const_iterator = typename vector_type::const_iterator;
		using size_type = typename vector_type::size_type;

	private:
		vector_type data;

	public:
		// Wrapped functions
		inline void reserve(usz nr) { data.reserve(nr); }
		inline void clear() { data.clear(); }
		inline size_type size() const { return data.size(); }
		inline bool empty() const { return data.empty(); }
		inline address_range& operator[](size_type n) { return data[n]; }
		inline const address_range& operator[](size_type n) const { return data[n]; }
		inline iterator begin() { return data.begin(); }
		inline const_iterator begin() const { return data.begin(); }
		inline iterator end() { return data.end(); }
		inline const_iterator end() const { return data.end(); }

		// Search for ranges that touch new_range. If found, merge instead of adding new_range.
		// When adding a new range, re-use invalid ranges whenever possible
		void merge(const address_range &new_range)
		{
			// Note the case where we have
			//   AAAA  BBBB
			//      CCCC
			// If we have data={A,B}, and new_range=C, we have to merge A with C, then B with A and invalidate B

			if (!new_range.valid())
			{
				return;
			}

			address_range *found = nullptr;
			address_range *invalid = nullptr;

			for (auto &existing : data)
			{
				if (!existing.valid())
				{
					invalid = &existing;
					continue;
				}

				// range1 overlaps, is immediately before, or is immediately after range2
				if (existing.touches(new_range))
				{
					if (found != nullptr)
					{
						// Already found a match, merge and invalidate "existing"
						found->set_min_max(existing);
						existing.invalidate();
					}
					else
					{
						// First match, merge "new_range"
						existing.set_min_max(new_range);
						found = &existing;
					}
				}
			}

			if (found != nullptr)
			{
				return;
			}

			if (invalid != nullptr)
			{
				*invalid = new_range;
			}
			else
			{
				data.push_back(new_range);
			}

			AUDIT(check_consistency());
		}

		void merge(const address_range_vector &other)
		{
			for (const address_range &new_range : other)
			{
				merge(new_range);
			}
		}

		// Exclude a given range from data
		void exclude(const address_range &exclusion)
		{
			// Note the case where we have
			//    AAAAAAA
			//      EEE
			// where data={A} and exclusion=E.
			// In this case, we need to reduce A to the head (before E starts), and then create a new address_range B for the tail (after E ends), i.e.
			//    AA   BB
			//      EEE

			if (!exclusion.valid())
			{
				return;
			}

			address_range *invalid = nullptr; // try to re-use an invalid range instead of calling push_back

			// We use index access because we might have to push_back within the loop, which could invalidate the iterators
			size_type _size = data.size();
			for (size_type n = 0; n < _size; ++n)
			{
				address_range &existing = data[n];

				if (!existing.valid())
				{
					// Null
					invalid = &existing;
					continue;
				}

				if (!existing.overlaps(exclusion))
				{
					// No overlap, skip
					continue;
				}

				const bool head_excluded = exclusion.overlaps(existing.start); // This section has its start inside excluded range
				const bool tail_excluded = exclusion.overlaps(existing.end); // This section has its end inside excluded range

				if (head_excluded && tail_excluded)
				{
					// Cannot be salvaged, fully excluded
					existing.invalidate();
					invalid = &existing;
				}
				else if (head_excluded)
				{
					// Head overlaps, truncate head
					existing.start = exclusion.next_address();
				}
				else if (tail_excluded)
				{
					// Tail overlaps, truncate tail
					existing.end = exclusion.prev_address();
				}
				else
				{
					// Section sits in the middle (see comment above function header)
					AUDIT(exclusion.inside(existing));
					const auto tail_end = existing.end;

					// Head
					existing.end = exclusion.prev_address();

					// Tail
					if (invalid != nullptr)
					{
						invalid->start = exclusion.next_address();
						invalid->end = tail_end;
						invalid = nullptr;
					}
					else
					{
						// IMPORTANT: adding to data invalidates "existing". This must be done last!
						data.push_back(address_range::start_end(exclusion.next_address(), tail_end));
					}
				}
			}
			AUDIT(check_consistency());
			AUDIT(!overlaps(exclusion));
		}

		void exclude(const address_range_vector &other)
		{
			for (const address_range &exclusion : other)
			{
				exclude(exclusion);
			}
		}

		// Checks the consistency of this vector.
		// Will fail if ranges within the vector overlap our touch each-other
		bool check_consistency() const
		{
			usz _size = data.size();

			for (usz i = 0; i < _size; ++i)
			{
				const auto &r1 = data[i];
				if (!r1.valid())
				{
					continue;
				}

				for (usz j = i + 1; j < _size; ++j)
				{
					const auto &r2 = data[j];
					if (!r2.valid())
					{
						continue;
					}

					if (r1.touches(r2))
					{
						return false;
					}
				}
			}
			return true;
		}

		// Test for overlap with a given range
		bool overlaps(const address_range &range) const
		{
			for (const address_range &current : data)
			{
				if (!current.valid())
				{
					continue;
				}

				if (current.overlaps(range))
				{
					return true;
				}
			}

			return false;
		}

		// Test for overlap with a given address_range vector
		bool overlaps(const address_range_vector &other) const
		{
			for (const address_range &rng1 : data)
			{
				if (!rng1.valid())
				{
					continue;
				}

				for (const address_range &rng2 : other.data)
				{
					if (!rng2.valid())
					{
						continue;
					}

					if (rng1.overlaps(rng2))
					{
						return true;
					}
				}
			}
			return false;
		}

		// Test if a given range is fully contained inside this vector
		bool contains(const address_range &range) const
		{
			for (const address_range &cur : *this)
			{
				if (!cur.valid())
				{
					continue;
				}

				if (range.inside(cur))
				{
					return true;
				}
			}
			return false;
		}

		// Test if all ranges in this vector are full contained inside a specific range
		bool inside(const address_range &range) const
		{
			for (const address_range &cur : *this)
			{
				if (!cur.valid())
				{
					continue;
				}

				if (!cur.inside(range))
				{
					return false;
				}
			}
			return true;
		}
	};


	// These declarations must be done after address_range_vector has been defined
	bool address_range::inside(const address_range_vector &vec) const
	{
		return vec.contains(*this);
	}

	bool address_range::overlaps(const address_range_vector &vec) const
	{
		return vec.overlaps(*this);
	}

} // namespace utils


namespace std
{
	static_assert(sizeof(usz) >= 2 * sizeof(u32), "usz must be at least twice the size of u32");

	template <>
	struct hash<utils::address_range>
	{
		usz operator()(const utils::address_range& k) const
		{
			// we can guarantee a unique hash since our type is 64 bits and usz as well
			return (usz{ k.start } << 32) | usz{ k.end };
		}
	};
}
