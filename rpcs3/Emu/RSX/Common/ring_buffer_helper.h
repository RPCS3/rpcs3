#pragma once

/**
 * Ring buffer memory helper :
 * There are 2 "pointers" (offset inside a memory buffer to be provided by class derrivative)
 * PUT pointer "points" to the start of allocatable space.
 * GET pointer "points" to the start of memory in use by the GPU.
 * Space between GET and PUT is used by the GPU ; this structure check that this memory is not overwritten.
 * User has to update the GET pointer when synchronisation happens.
 */
struct data_heap
{
	/**
	* Does alloc cross get position ?
	*/
	template<int Alignement>
	bool can_alloc(size_t size) const
	{
		size_t alloc_size = align(size, Alignement);
		size_t aligned_put_pos = align(m_put_pos, Alignement);
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

	size_t m_size;
	size_t m_put_pos; // Start of free space
public:
	data_heap() = default;
	~data_heap() = default;
	data_heap(const data_heap&) = delete;
	data_heap(data_heap&&) = delete;

	size_t m_get_pos; // End of free space

	void init(size_t heap_size)
	{
		m_size = heap_size;
		m_put_pos = 0;
		m_get_pos = heap_size - 1;
	}

	template<int Alignement>
	size_t alloc(size_t size)
	{
		if (!can_alloc<Alignement>(size)) throw EXCEPTION("Working buffer not big enough");
		size_t alloc_size = align(size, Alignement);
		size_t aligned_put_pos = align(m_put_pos, Alignement);
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
	size_t get_current_put_pos_minus_one() const
	{
		return (m_put_pos - 1 > 0) ? m_put_pos - 1 : m_size - 1;
	}
};
