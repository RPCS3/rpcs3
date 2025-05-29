#include "stdafx.h"
#include "ring_buffer.h"

namespace gl
{
	void ring_buffer::recreate(GLsizeiptr size, const void* data)
	{
		if (m_id)
		{
			m_fence.wait_for_signal();
			remove();
		}

		buffer::create();
		save_binding_state save(current_target(), *this);

		GLbitfield buffer_storage_flags = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
		if (gl::get_driver_caps().vendor_MESA) buffer_storage_flags |= GL_CLIENT_STORAGE_BIT;

		DSA_CALL2(NamedBufferStorage, m_id, size, data, buffer_storage_flags);
		m_memory_mapping = DSA_CALL2_RET(MapNamedBufferRange, m_id, 0, size, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);

		ensure(m_memory_mapping != nullptr);
		m_data_loc = 0;
		m_size = ::narrow<u32>(size);
		m_memory_type = memory_type::host_visible;
	}

	void ring_buffer::create(target target_, GLsizeiptr size, const void* data_)
	{
		m_target = target_;
		recreate(size, data_);
	}

	std::pair<void*, u32> ring_buffer::alloc_from_heap(u32 alloc_size, u16 alignment)
	{
		u32 offset = m_data_loc;
		if (m_data_loc) offset = utils::align(offset, alignment);

		if ((offset + alloc_size) > m_size)
		{
			if (!m_fence.is_empty())
			{
				m_fence.wait_for_signal();
			}
			else
			{
				rsx_log.error("OOM Error: Ring buffer was likely being used without notify() being called");
				glFinish();
			}

			m_data_loc = 0;
			offset = 0;
		}

		//Align data loc to 256; allows some "guard" region so we dont trample our own data inadvertently
		m_data_loc = utils::align(offset + alloc_size, 256);
		return std::make_pair(static_cast<char*>(m_memory_mapping) + offset, offset);
	}

	void ring_buffer::remove()
	{
		if (m_memory_mapping)
		{
			buffer::unmap();

			m_memory_mapping = nullptr;
			m_data_loc = 0;
			m_size = 0;
		}

		buffer::remove();
	}

	void ring_buffer::notify()
	{
		//Insert fence about 25% into the buffer
		if (m_fence.is_empty() && (m_data_loc > (m_size >> 2)))
			m_fence.reset();
	}

	// Legacy ring buffer - used when ARB_buffer_storage is not available, OR when capturing with renderdoc
	void legacy_ring_buffer::recreate(GLsizeiptr size, const void* data)
	{
		if (m_id)
			remove();

		buffer::create();
		buffer::data(size, data, GL_DYNAMIC_DRAW);

		m_memory_type = memory_type::host_visible;
		m_memory_mapping = nullptr;
		m_data_loc = 0;
		m_size = ::narrow<u32>(size);
	}

	void legacy_ring_buffer::create(target target_, GLsizeiptr size, const void* data_)
	{
		m_target = target_;
		recreate(size, data_);
	}

	void legacy_ring_buffer::reserve_storage_on_heap(u32 alloc_size)
	{
		ensure(m_memory_mapping == nullptr);

		u32 offset = m_data_loc;
		if (m_data_loc) offset = utils::align(offset, 256);

		const u32 block_size = utils::align(alloc_size + 16, 256);	//Overallocate just in case we need to realign base

		if ((offset + block_size) > m_size)
		{
			buffer::data(m_size, nullptr, GL_DYNAMIC_DRAW);
			m_data_loc = 0;
		}

		m_memory_mapping = DSA_CALL2_RET(MapNamedBufferRange, m_id, m_data_loc, block_size, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
		m_mapped_bytes = block_size;
		m_mapping_offset = m_data_loc;
		m_alignment_offset = 0;

		//When using debugging tools, the mapped base might not be aligned as expected
		const u64 mapped_address_base = reinterpret_cast<u64>(m_memory_mapping);
		if (mapped_address_base & 0xF)
		{
			//Unaligned result was returned. We have to modify the base address a bit
			//We lose some memory here, but the 16 byte overallocation above makes up for it
			const u64 new_base = (mapped_address_base & ~0xF) + 16;
			const u64 diff_bytes = new_base - mapped_address_base;

			m_memory_mapping = reinterpret_cast<void*>(new_base);
			m_mapped_bytes -= ::narrow<u32>(diff_bytes);
			m_alignment_offset = ::narrow<u32>(diff_bytes);
		}

		ensure(m_mapped_bytes >= alloc_size);
	}

	std::pair<void*, u32> legacy_ring_buffer::alloc_from_heap(u32 alloc_size, u16 alignment)
	{
		u32 offset = m_data_loc;
		if (m_data_loc) offset = utils::align(offset, alignment);

		u32 padding = (offset - m_data_loc);
		u32 real_size = utils::align(padding + alloc_size, alignment);	//Ensures we leave the loc pointer aligned after we exit

		if (real_size > m_mapped_bytes)
		{
			//Missed allocation. We take a performance hit on doing this.
			//Overallocate slightly for the next allocation if requested size is too small
			unmap();
			reserve_storage_on_heap(std::max(real_size, 4096U));

			offset = m_data_loc;
			if (m_data_loc) offset = utils::align(offset, alignment);

			padding = (offset - m_data_loc);
			real_size = utils::align(padding + alloc_size, alignment);
		}

		m_data_loc = offset + real_size;
		m_mapped_bytes -= real_size;

		u32 local_offset = (offset - m_mapping_offset);
		return std::make_pair(static_cast<char*>(m_memory_mapping) + local_offset, offset + m_alignment_offset);
	}

	void legacy_ring_buffer::remove()
	{
		ring_buffer::remove();
		m_mapped_bytes = 0;
	}

	void legacy_ring_buffer::unmap()
	{
		buffer::unmap();

		m_memory_mapping = nullptr;
		m_mapped_bytes = 0;
		m_mapping_offset = 0;
	}

	// AMD persistent mapping workaround for driver-assisted flushing
	void* transient_ring_buffer::map_internal(u32 offset, u32 length)
	{
		flush();

		dirty = true;
		return DSA_CALL2_RET(MapNamedBufferRange, m_id, offset, length, GL_MAP_WRITE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
	}

	void transient_ring_buffer::bind()
	{
		flush();
		buffer::bind();
	}

	void transient_ring_buffer::recreate(GLsizeiptr size, const void* data)
	{
		if (m_id)
		{
			m_fence.wait_for_signal();
			remove();
		}

		buffer::create();
		save_binding_state save(current_target(), *this);
		DSA_CALL2(NamedBufferStorage, m_id, size, data, GL_MAP_WRITE_BIT);

		m_data_loc = 0;
		m_size = ::narrow<u32>(size);
		m_memory_type = memory_type::host_visible;
	}

	std::pair<void*, u32> transient_ring_buffer::alloc_from_heap(u32 alloc_size, u16 alignment)
	{
		ensure(m_memory_mapping == nullptr);
		const auto allocation = ring_buffer::alloc_from_heap(alloc_size, alignment);
		return { map_internal(allocation.second, alloc_size), allocation.second };
	}

	void transient_ring_buffer::flush()
	{
		if (dirty)
		{
			buffer::unmap();
			dirty = false;
		}
	}

	void transient_ring_buffer::unmap()
	{
		flush();
	}

	scratch_ring_buffer::~scratch_ring_buffer()
	{
		if (m_storage)
		{
			remove();
		}
	}

	void scratch_ring_buffer::create(buffer::target target_, u64 size, u32 usage_flags)
	{
		if (m_storage)
		{
			remove();
		}

		m_storage.create(target_, size, nullptr, gl::buffer::memory_type::local, usage_flags);
	}

	void scratch_ring_buffer::remove()
	{
		if (m_storage)
		{
			m_storage.remove();
		}

		m_barriers.clear();
		m_alloc_pointer = 0;
	}

	u32 scratch_ring_buffer::alloc(u32 size, u32 alignment)
	{
		u64 start = utils::align(m_alloc_pointer, alignment);
		m_alloc_pointer = (start + size);

		if (static_cast<GLsizeiptr>(m_alloc_pointer) > m_storage.size())
		{
			start = 0;
			m_alloc_pointer = size;
		}

		pop_barrier(static_cast<u32>(start), size);
		return static_cast<u32>(start);
	}

	void scratch_ring_buffer::pop_barrier(u32 start, u32 length)
	{
		const auto range = utils::address_range32::start_length(start, length);
		m_barriers.erase(std::remove_if(m_barriers.begin(), m_barriers.end(), [&range](auto& barrier_)
		{
			if (barrier_.range.overlaps(range))
			{
				barrier_.signal.server_wait_sync();
				barrier_.signal.destroy();
				return true;
			}

			return false;
		}), m_barriers.end());
	}

	void scratch_ring_buffer::push_barrier(u32 start, u32 length)
	{
		if (!length)
		{
			return;
		}

		barrier barrier_;
		barrier_.range = utils::address_range32::start_length(start, length);
		barrier_.signal.create();
		m_barriers.emplace_back(barrier_);
	}
}
