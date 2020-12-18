#pragma once

#include "util/logs.hpp"
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
	* Does alloc cross get position ?
	*/
	template<int Alignment>
	bool can_alloc(usz size) const
	{
		usz alloc_size = utils::align(size, Alignment);
		usz aligned_put_pos = utils::align(m_put_pos, Alignment);
		if (aligned_put_pos + alloc_size < m_size)
		{
			// range before get
			if (aligned_put_pos + alloc_size < m_get_pos)
				return true;
			// range after get
			if (aligned_put_pos > m_get_pos)
				return true;
			return false;
		}
		else
		{
			// ..]....[..get..
			if (aligned_put_pos < m_get_pos)
				return false;
			// ..get..]...[...
			// Actually all resources extending beyond heap space starts at 0
			if (alloc_size > m_get_pos)
				return false;
			return true;
		}
	}

    // Grow the buffer to hold at least size bytes
	virtual bool grow(usz /*size*/)
	{
		// Stub
		return false;
	}

	usz m_size;
	usz m_put_pos; // Start of free space
	usz m_min_guard_size; //If an allocation touches the guard region, reset the heap to avoid going over budget
	usz m_current_allocated_size;
	usz m_largest_allocated_pool;

	char* m_name;
public:
	data_heap() = default;
	~data_heap() = default;
	data_heap(const data_heap&) = delete;
	data_heap(data_heap&&) = delete;

	usz m_get_pos; // End of free space

	void init(usz heap_size, const char* buffer_name = "unnamed", usz min_guard_size=0x10000)
	{
		m_name = const_cast<char*>(buffer_name);

		m_size = heap_size;
		m_put_pos = 0;
		m_get_pos = heap_size - 1;

		//allocation stats
		m_min_guard_size = min_guard_size;
		m_current_allocated_size = 0;
		m_largest_allocated_pool = 0;
	}

	template<int Alignment>
	usz alloc(usz size)
	{
		const usz alloc_size = utils::align(size, Alignment);
		const usz aligned_put_pos = utils::align(m_put_pos, Alignment);

		if (!can_alloc<Alignment>(size) && !grow(aligned_put_pos + alloc_size))
		{
			fmt::throw_exception("[%s] Working buffer not big enough, buffer_length=%d allocated=%d requested=%d guard=%d largest_pool=%d",
					m_name, m_size, m_current_allocated_size, size, m_min_guard_size, m_largest_allocated_pool);
		}

		const usz block_length = (aligned_put_pos - m_put_pos) + alloc_size;
		m_current_allocated_size += block_length;
		m_largest_allocated_pool = std::max(m_largest_allocated_pool, block_length);

		if (aligned_put_pos + alloc_size < m_size)
		{
			m_put_pos = aligned_put_pos + alloc_size;
			return aligned_put_pos;
		}
		else
		{
			m_put_pos = alloc_size;
			return 0;
		}
	}

	/**
	* return current putpos - 1
	*/
	usz get_current_put_pos_minus_one() const
	{
		return (m_put_pos > 0) ? m_put_pos - 1 : m_size - 1;
	}

	virtual bool is_critical() const
	{
		const usz guard_length = std::max(m_min_guard_size, m_largest_allocated_pool);
		return (m_current_allocated_size + guard_length) >= m_size;
	}

	void reset_allocation_stats()
	{
		m_current_allocated_size = 0;
		m_largest_allocated_pool = 0;
		m_get_pos = get_current_put_pos_minus_one();
	}

	// Updates the current_allocated_size metrics
	void notify()
	{
		if (m_get_pos == UINT64_MAX)
			m_current_allocated_size = 0;
		else if (m_get_pos < m_put_pos)
			m_current_allocated_size = (m_put_pos - m_get_pos - 1);
		else if (m_get_pos > m_put_pos)
			m_current_allocated_size = (m_put_pos + (m_size - m_get_pos - 1));
		else
			fmt::throw_exception("m_put_pos == m_get_pos!");
	}

	usz size() const
	{
		return m_size;
	}
};
