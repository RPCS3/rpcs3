#include "memory_block.h"

namespace vk
{
	memory_block::memory_block(VkDevice dev, u64 block_sz, u64 alignment, u32 memory_type_index)
		: m_device(dev)
	{
		m_mem_allocator = get_current_mem_allocator();
		m_mem_handle    = m_mem_allocator->alloc(block_sz, alignment, memory_type_index);
	}

	memory_block::~memory_block()
	{
		m_mem_allocator->free(m_mem_handle);
	}

	VkDeviceMemory memory_block::get_vk_device_memory()
	{
		return m_mem_allocator->get_vk_device_memory(m_mem_handle);
	}

	u64 memory_block::get_vk_device_memory_offset()
	{
		return m_mem_allocator->get_vk_device_memory_offset(m_mem_handle);
	}

	void* memory_block::map(u64 offset, u64 size)
	{
		return m_mem_allocator->map(m_mem_handle, offset, size);
	}

	void memory_block::unmap()
	{
		m_mem_allocator->unmap(m_mem_handle);
	}
}
