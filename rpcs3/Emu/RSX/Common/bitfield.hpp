#pragma once

#include <util/types.hpp>
#include <bitset>

namespace rsx
{
	template <int N>
	void unpack_bitset(const std::bitset<N>& block, u64* values)
	{
		for (int bit = 0, n = -1, shift = 0; bit < N; ++bit, ++shift)
		{
			if ((bit % 64) == 0)
			{
				values[++n] = 0;
				shift = 0;
			}

			if (block[bit])
			{
				values[n] |= (1ull << shift);
			}
		}
	}

	template <int N>
	void pack_bitset(std::bitset<N>& block, u64* values)
	{
		for (int n = 0, shift = 0; shift < N; ++n, shift += 64)
		{
			std::bitset<N> tmp = values[n];
			tmp <<= shift;
			block |= tmp;
		}
	}

	template <typename T, typename bitmask_type = u32>
	class atomic_bitmask_t
	{
	private:
		atomic_t<bitmask_type> m_data{ 0 };

	public:
		atomic_bitmask_t() = default;

		T load() const
		{
			return static_cast<T>(m_data.load());
		}

		void store(T value)
		{
			m_data.store(static_cast<bitmask_type>(value));
		}

		bool operator & (T mask) const
		{
			return ((m_data.load() & static_cast<bitmask_type>(mask)) != 0);
		}

		T operator | (T mask) const
		{
			return static_cast<T>(m_data.load() | static_cast<bitmask_type>(mask));
		}

		void operator &= (T mask)
		{
			m_data.fetch_and(static_cast<bitmask_type>(mask));
		}

		void operator |= (T mask)
		{
			m_data.fetch_or(static_cast<bitmask_type>(mask));
		}

		bool test_and_set(T mask)
		{
			const auto old = m_data.fetch_or(static_cast<bitmask_type>(mask));
			return (old & static_cast<bitmask_type>(mask)) != 0;
		}

		auto clear(T mask)
		{
			bitmask_type clear_mask = ~(static_cast<bitmask_type>(mask));
			return m_data.and_fetch(clear_mask);
		}

		void clear()
		{
			m_data.release(0);
		}
	};
}
