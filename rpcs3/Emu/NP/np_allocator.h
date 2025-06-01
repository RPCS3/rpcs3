#pragma once

#include <map>

#include "Emu/Memory/vm_ptr.h"
#include "Utilities/mutex.h"
#include "util/asm.hpp"
#include "util/logs.hpp"

LOG_CHANNEL(np_mem_allocator);

namespace np
{
	class memory_allocator
	{
	public:
		memory_allocator() = default;
		memory_allocator(utils::serial& ar) noexcept { save(ar); }
		memory_allocator(const memory_allocator&) = delete;
		memory_allocator& operator=(const memory_allocator&) = delete;

		void save(utils::serial& ar);

		void setup(vm::ptr<void> ptr_pool, u32 size)
		{
			std::lock_guard lock(m_mutex);
			m_pool  = ptr_pool;
			m_size  = size;
			m_avail = size;
			m_allocs.clear();
		}

		void release()
		{
			std::lock_guard lock(m_mutex);
			m_pool  = vm::null;
			m_size  = 0;
			m_avail = 0;
			m_allocs.clear();
		}

		u32 allocate(u32 size)
		{
			std::lock_guard lock(m_mutex);
			if (!size)
			{
				np_mem_allocator.error("Can't allocate 0 size buffer!");
				return 0;
			}

			// Align allocs
			const u32 alloc_size = utils::align(size, 4);
			if (alloc_size > m_avail)
			{
				np_mem_allocator.error("Not enough memory available in NP pool!");
				return 0;
			}

			u32 last_free    = 0;
			bool found_space = false;

			for (const auto& a : m_allocs)
			{
				if ((a.first - last_free) >= alloc_size)
				{
					found_space = true;
					break;
				}

				last_free = a.first + a.second;
			}

			if (!found_space)
			{
				if ((m_size - last_free) < alloc_size)
				{
					np_mem_allocator.error("Not enough memory available in NP pool(continuous block)!");
					return 0;
				}
			}

			m_allocs.emplace(last_free, alloc_size);
			m_avail -= alloc_size;

			memset((static_cast<u8*>(m_pool.get_ptr())) + last_free, 0, alloc_size);

			np_mem_allocator.trace("Allocation off:%d size:%d psize:%d, pavail:%d", last_free, alloc_size, m_size, m_avail);

			return m_pool.addr() + last_free;
		}

		void free(u32 addr)
		{
			std::lock_guard lock(m_mutex);

			ensure(addr >= m_pool.addr() && addr < (m_pool.addr() + m_size), "memory_allocator::free: addr is out of bounds!");

			const u32 offset = addr - m_pool.addr();
			ensure(m_allocs.contains(offset), "memory_allocator::free: m_allocs doesn't contain the allocation!");

			m_avail += ::at32(m_allocs, offset);
			m_allocs.erase(offset);
		}

		void shrink_allocation(u32 addr, u32 new_size)
		{
			std::lock_guard lock(m_mutex);

			ensure(addr >= m_pool.addr() && addr < (m_pool.addr() + m_size), "memory_allocator::reduce_allocation: addr is out of bounds!");

			const u32 offset = addr - m_pool.addr();
			ensure(m_allocs.contains(offset), "memory_allocator::reduce_allocation: m_allocs doesn't contain the allocation!");
			ensure(m_allocs[offset] >= new_size, "memory_allocator::reduce_allocation: New size is bigger than current allocation!");

			m_avail += (::at32(m_allocs, offset) - new_size);
			m_allocs[offset] = new_size;
		}

	private:
		shared_mutex m_mutex;
		vm::ptr<void> m_pool{};
		u32 m_size  = 0;
		u32 m_avail = 0;
		std::map<u32, u32> m_allocs{}; // offset/size
	};
} // namespace np
