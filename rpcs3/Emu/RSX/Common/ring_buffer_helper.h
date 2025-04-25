#pragma once

#include "Utilities/StrFmt.h"
#include "util/asm.hpp"

/**
 * Ring buffer memory helper :
 * There are 2 "pointers" (offset inside a memory buffer to be provided by class derivative)
 * PUT pointer "points" to the start of allocatable space.
 * GET pointer "points" to the start of memory in use by the GPU.
 * Space between GET and PUT is used by the GPU ; this structure check that this memory is not overwritten.
 * User has to update the GET pointer when synchronisation happens.
 */
class data_heap
{
protected:
	/**
	* Internal implementation of allocation test
	* Does alloc cross get position?
	*/
	bool can_alloc_impl(usz aligned_put_pos, usz aligned_alloc_size) const
	{
		const usz alloc_end = aligned_put_pos + aligned_alloc_size;
		if (alloc_end < m_size) [[ likely ]]
		{
			// Range before get
			if (alloc_end < m_get_pos)
				return true;

			// Range after get
			if (aligned_put_pos > m_get_pos)
				return true;

			return false;
		}

		// ..]....[..get..
		if (aligned_put_pos < m_get_pos)
			return false;

		// ..get..]...[...
		// Actually all resources extending beyond heap space starts at 0
		if (aligned_alloc_size > m_get_pos)
			return false;

		return true;
	}

	/**
	* Does alloc cross get position?
	*/
	template<int Alignment>
	bool can_alloc(usz size) const
	{
		const usz alloc_size = utils::align(size, Alignment);
		const usz aligned_put_pos = utils::align(m_put_pos, Alignment);
		return can_alloc_impl(aligned_put_pos, alloc_size);
	}

	// Grow the buffer to hold at least size bytes
	virtual bool grow(usz /*size*/)
	{
		// Stub
		return false;
	}

	usz m_size;
	usz m_put_pos;                 // Start of free space
	usz m_get_pos;                 // End of free space
	usz m_min_guard_size;          // If an allocation touches the guard region, reset the heap to avoid going over budget

	char* m_name;
public:
	data_heap() = default;
	~data_heap() = default;
	data_heap(const data_heap&) = delete;
	data_heap(data_heap&&) = delete;

	void init(usz heap_size, const char* buffer_name = "unnamed", usz min_guard_size=0x10000)
	{
		m_name = const_cast<char*>(buffer_name);

		m_size = heap_size;
		m_put_pos = 0;
		m_get_pos = heap_size - 1;

		// Allocation stats
		m_min_guard_size = min_guard_size;
	}

	template<int Alignment>
	usz alloc(usz size)
	{
		const usz alloc_size = utils::align(size, Alignment);
		const usz aligned_put_pos = utils::align(m_put_pos, Alignment);

		if (!can_alloc<Alignment>(size) && !grow(alloc_size))
		{
			fmt::throw_exception("[%s] Working buffer not big enough, buffer_length=%d requested=%d guard=%d",
					m_name, m_size, size, m_min_guard_size);
		}

		const usz alloc_end = aligned_put_pos + alloc_size;
		if (alloc_end < m_size)
		{
			m_put_pos = alloc_end;
			return aligned_put_pos;
		}

		m_put_pos = alloc_size;
		return 0;
	}

	/*
	 * For use in cases where we take a fixed amount each time
	 */
	template<int Alignment, usz Size = Alignment>
	usz static_alloc()
	{
		static_assert((Size & (Alignment - 1)) == 0);
		ensure((m_put_pos & (Alignment - 1)) == 0);

		if (!can_alloc_impl(m_put_pos, Size) && !grow(Size))
		{
			fmt::throw_exception("[%s] Working buffer not big enough, buffer_length=%d requested=%d guard=%d",
					m_name, m_size, Size, m_min_guard_size);
		}

		const usz alloc_end = m_put_pos + Size;
		if (alloc_end < m_size)
		{
			const auto ret_pos = m_put_pos;
			m_put_pos = alloc_end;
			return ret_pos;
		}

		m_put_pos = Size;
		return 0;
	}

	/**
	* return current putpos - 1
	*/
	usz get_current_put_pos_minus_one() const
	{
		return (m_put_pos > 0) ? m_put_pos - 1 : m_size - 1;
	}

	inline void set_get_pos(usz value)
	{
		m_get_pos = value;
	}

	void reset_allocation_stats()
	{
		m_get_pos = get_current_put_pos_minus_one();
	}

	// Updates the current_allocated_size metrics
	inline void notify()
	{
		// @unused
	}

	usz size() const
	{
		return m_size;
	}
};
